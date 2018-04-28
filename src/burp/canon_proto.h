/*
 *	PROGRAM:	JRD Backup and Restore Program
 *	MODULE:		canon_proto.h
 *	DESCRIPTION:	Prototype Header file for canonical.cpp
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
 */

#ifndef BURP_CANON_PROTO_H
#define BURP_CANON_PROTO_H

ULONG	CAN_encode_decode (burp_rel* relation, lstring* buffer, UCHAR* data, bool direction, bool useMissingOffset = false);
ULONG	CAN_slice (lstring* buffer, lstring* slice, bool direction, UCHAR* sdl);

#endif	// BURP_CANON_PROTO_H

