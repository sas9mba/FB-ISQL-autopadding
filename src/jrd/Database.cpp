/*
 *      PROGRAM:        JRD access method
 *      MODULE:         Database.cpp
 *      DESCRIPTION:    Common descriptions
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * Sean Leyne
 * Claudio Valderrama C.
 */

#include "firebird.h"
// Definition of block types for data allocation in JRD
#include "../include/fb_blk.h"

#include "../jrd/ods.h"
#include "../jrd/lck.h"
#include "../jrd/Database.h"
#include "../jrd/nbak.h"
#include "../jrd/tra.h"
#include "../jrd/tpc_proto.h"
#include "../jrd/lck_proto.h"
#include "../jrd/CryptoManager.h"
#include "../jrd/os/pio_proto.h"
#include "../common/os/os_utils.h"

// Thread data block
#include "../common/ThreadData.h"

using namespace Firebird;

namespace Jrd
{
	bool Database::onRawDevice() const
	{
		const PageSpace* const pageSpace = dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
		return pageSpace->onRawDevice();
	}

	string Database::getUniqueFileId() const
	{
		const PageSpace* const pageSpace = dbb_page_manager.findPageSpace(DB_PAGE_SPACE);

		UCharBuffer buffer;
		os_utils::getUniqueFileId(pageSpace->file->fil_desc, buffer);

		string file_id;
		char* s = file_id.getBuffer(2 * buffer.getCount());
		for (FB_SIZE_T i = 0; i < buffer.getCount(); i++)
		{
			sprintf(s, "%02x", (int) buffer[i]);
			s += 2;
		}

		return file_id;
	}

	Database::~Database()
	{
		if (dbb_linger_timer)
		{
			dbb_linger_timer->destroy();
		}

		{ // scope
			SyncLockGuard guard(&dbb_sortbuf_sync, SYNC_EXCLUSIVE, "Database::~Database");

			while (dbb_sort_buffers.hasData())
				delete[] dbb_sort_buffers.pop();
		}

		{ // scope
			SyncLockGuard guard(&dbb_pools_sync, SYNC_EXCLUSIVE, "Database::~Database");

			fb_assert(dbb_pools[0] == dbb_permanent);

			for (FB_SIZE_T i = 1; i < dbb_pools.getCount(); ++i)
				MemoryPool::deletePool(dbb_pools[i]);
		}

		delete dbb_monitoring_data;
		delete dbb_backup_manager;
		delete dbb_crypto_manager;
		delete dbb_tip_cache;

		fb_assert(!locked());
		// This line decrements the usage counter and may cause the destructor to be called.
		// It should happen with the dbb_sync unlocked.
		LockManager::destroy(dbb_lock_mgr);
		EventManager::destroy(dbb_event_mgr);
	}

	void Database::deletePool(MemoryPool* pool)
	{
		if (pool)
		{
			{
				SyncLockGuard guard(&dbb_pools_sync, SYNC_EXCLUSIVE, "Database::deletePool");
				FB_SIZE_T pos;

				if (dbb_pools.find(pool, pos))
					dbb_pools.remove(pos);
			}

			MemoryPool::deletePool(pool);
		}
	}

	int Database::blocking_ast_sweep(void* ast_object)
	{
		try
		{
			Database* dbb = static_cast<Database*>(ast_object);
			AsyncContextHolder tdbb(dbb, FB_FUNCTION);

			if ((dbb->dbb_flags & DBB_sweep_starting) && !(dbb->dbb_flags & DBB_sweep_in_progress))
			{
				dbb->dbb_flags &= ~DBB_sweep_starting;
				LCK_release(tdbb, dbb->dbb_sweep_lock);
			}
		}
		catch (const Exception&)
		{} // no-op

		return 0;
	}

	Lock* Database::createSweepLock(thread_db* tdbb)
	{
		if (!dbb_sweep_lock)
		{
			dbb_sweep_lock = FB_NEW_RPT(*dbb_permanent, 0)
				Lock(tdbb, 0, LCK_sweep, this, blocking_ast_sweep);
		}

		return dbb_sweep_lock;
	}

	bool Database::allowSweepThread(thread_db* tdbb)
	{
		if (readOnly())
			return false;

		Jrd::Attachment* const attachment = tdbb->getAttachment();
		if (attachment->att_flags & ATT_no_cleanup)
			return false;

		while (true)
		{
			AtomicCounter::counter_type old = dbb_flags;
			if ((old & (DBB_sweep_in_progress | DBB_sweep_starting)) || (dbb_ast_flags & DBB_shutdown))
				return false;

			if (dbb_flags.compareExchange(old, old | DBB_sweep_starting))
				break;
		}

		createSweepLock(tdbb);
		if (!LCK_lock(tdbb, dbb_sweep_lock, LCK_EX, LCK_NO_WAIT))
		{
			// clear lock error from status vector
			fb_utils::init_status(tdbb->tdbb_status_vector);

			dbb_flags &= ~DBB_sweep_starting;
			return false;
		}

		return true;
	}

	bool Database::allowSweepRun(thread_db* tdbb)
	{
		if (readOnly())
			return false;

		Jrd::Attachment* const attachment = tdbb->getAttachment();
		if (attachment->att_flags & ATT_no_cleanup)
			return false;

		while (true)
		{
			AtomicCounter::counter_type old = dbb_flags;
			if (old & DBB_sweep_in_progress)
				return false;

			if (dbb_flags.compareExchange(old, old | DBB_sweep_in_progress))
				break;
		}

		if (!(dbb_flags & DBB_sweep_starting))
		{
			createSweepLock(tdbb);
			if (!LCK_lock(tdbb, dbb_sweep_lock, LCK_EX, -1))
			{
				// clear lock error from status vector
				fb_utils::init_status(tdbb->tdbb_status_vector);

				dbb_flags &= ~DBB_sweep_in_progress;
				return false;
			}
		}
		else
			dbb_flags &= ~DBB_sweep_starting;

		return true;
	}

	void Database::clearSweepFlags(thread_db* tdbb)
	{
		if (!(dbb_flags & (DBB_sweep_starting | DBB_sweep_in_progress)))
			return;

		if (dbb_sweep_lock)
			LCK_release(tdbb, dbb_sweep_lock);

		dbb_flags &= ~(DBB_sweep_in_progress | DBB_sweep_starting);
	}

	void Database::registerModule(Module& module)
	{
		Sync sync(&dbb_modules_sync, FB_FUNCTION);
		sync.lock(SYNC_SHARED);
		if (dbb_modules.exist(module))
			return;

		sync.unlock();
		sync.lock(SYNC_EXCLUSIVE);
		if (!dbb_modules.exist(module))
			dbb_modules.add(module);
	}

	void Database::SharedCounter::shutdown(thread_db* tdbb)
	{
		for (size_t i = 0; i < TOTAL_ITEMS; i++)
		{
			if (m_counters[i].lock)
				LCK_release(tdbb, m_counters[i].lock);
		}
	}

	SINT64 Database::SharedCounter::generate(thread_db* tdbb, ULONG space, ULONG prefetch)
	{
		fb_assert(space < TOTAL_ITEMS);
		ValueCache* const counter = &m_counters[space];

		Database* const dbb = tdbb->getDatabase();
		SyncLockGuard guard(&dbb->dbb_sh_counter_sync, SYNC_EXCLUSIVE, "Database::SharedCounter::generate");

		SINT64 result = ++counter->curVal;

		if (!m_localOnly && result > counter->maxVal)
		{
			if (!counter->lock)
			{
				counter->lock =
					FB_NEW_RPT(*dbb->dbb_permanent, 0) Lock(tdbb, sizeof(SLONG), LCK_shared_counter);
				counter->lock->setKey(space);
				LCK_lock(tdbb, counter->lock, LCK_PW, LCK_WAIT);
			}
			else
				LCK_convert(tdbb, counter->lock, LCK_PW, LCK_WAIT);

			result = LCK_read_data(tdbb, counter->lock);

			// zero IDs are somewhat special, so let's better skip them
			if (!result)
				result = 1;

			counter->curVal = result;
			counter->maxVal = result + prefetch - 1;
			LCK_write_data(tdbb, counter->lock, counter->maxVal + 1);
			LCK_convert(tdbb, counter->lock, LCK_SR, LCK_WAIT);
		}

		return result;
	}

	void Database::Linger::handler()
	{
		JRD_shutdown_database(dbb, SHUT_DBB_RELEASE_POOLS);
	}

	int Database::Linger::release()
	{
		if (--refCounter == 0)
		{
			delete this;
			return 0;
		}

		return 1;
	}

	void Database::Linger::reset()
	{
		if (active)
		{
			FbLocalStatus s;
			TimerInterfacePtr()->stop(&s, this);
			if (!(s->getState() & IStatus::STATE_ERRORS))
				active = false;
		}
	}

	void Database::Linger::set(unsigned seconds)
	{
		if (dbb && !active)
		{
			FbLocalStatus s;
			TimerInterfacePtr()->start(&s, this, seconds * 1000 * 1000);
			check(&s);
			active = true;
		}
	}

	void Database::Linger::destroy()
	{
		dbb = NULL;
		reset();
	}

} // namespace
