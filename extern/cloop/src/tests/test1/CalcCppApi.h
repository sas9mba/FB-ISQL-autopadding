// This file was autogenerated by cloop - Cross Language Object Oriented Programming

#ifndef CALC_CPP_API_H
#define CALC_CPP_API_H

#ifndef CLOOP_CARG
#define CLOOP_CARG
#endif


namespace calc
{
	class DoNotInherit
	{
	};

	template <typename T>
	class Inherit : public T
	{
	public:
		Inherit(DoNotInherit = DoNotInherit())
			: T(DoNotInherit())
		{
		}
	};

	// Forward interfaces declarations

	class IDisposable;
	class IStatus;
	class IFactory;
	class ICalculator;
	class ICalculator2;

	// Interfaces declarations

	class IDisposable
	{
	public:
		struct VTable
		{
			void* cloopDummy[1];
			uintptr_t version;
			void (CLOOP_CARG *dispose)(IDisposable* self) throw();
		};

		void* cloopDummy[1];
		VTable* cloopVTable;

	protected:
		IDisposable(DoNotInherit)
		{
		}

		~IDisposable()
		{
		}

	public:
		static const unsigned VERSION = 1;

		void dispose()
		{
			static_cast<VTable*>(this->cloopVTable)->dispose(this);
		}
	};

	class IStatus : public IDisposable
	{
	public:
		struct VTable : public IDisposable::VTable
		{
			int (CLOOP_CARG *getCode)(const IStatus* self) throw();
			void (CLOOP_CARG *setCode)(IStatus* self, int code) throw();
		};

	protected:
		IStatus(DoNotInherit)
			: IDisposable(DoNotInherit())
		{
		}

		~IStatus()
		{
		}

	public:
		static const unsigned VERSION = 2;

		static const int ERROR_1 = 1;
		static const int ERROR_2 = 2;
		static const int ERROR_12 = IStatus::ERROR_1 | IStatus::ERROR_2;

		int getCode() const
		{
			int ret = static_cast<VTable*>(this->cloopVTable)->getCode(this);
			return ret;
		}

		void setCode(int code)
		{
			static_cast<VTable*>(this->cloopVTable)->setCode(this, code);
		}
	};

	class IFactory : public IDisposable
	{
	public:
		struct VTable : public IDisposable::VTable
		{
			IStatus* (CLOOP_CARG *createStatus)(IFactory* self) throw();
			ICalculator* (CLOOP_CARG *createCalculator)(IFactory* self, IStatus* status) throw();
			ICalculator2* (CLOOP_CARG *createCalculator2)(IFactory* self, IStatus* status) throw();
			ICalculator* (CLOOP_CARG *createBrokenCalculator)(IFactory* self, IStatus* status) throw();
		};

	protected:
		IFactory(DoNotInherit)
			: IDisposable(DoNotInherit())
		{
		}

		~IFactory()
		{
		}

	public:
		static const unsigned VERSION = 2;

		IStatus* createStatus()
		{
			IStatus* ret = static_cast<VTable*>(this->cloopVTable)->createStatus(this);
			return ret;
		}

		template <typename StatusType> ICalculator* createCalculator(StatusType* status)
		{
			StatusType::clearException(status);
			ICalculator* ret = static_cast<VTable*>(this->cloopVTable)->createCalculator(this, status);
			StatusType::checkException(status);
			return ret;
		}

		template <typename StatusType> ICalculator2* createCalculator2(StatusType* status)
		{
			StatusType::clearException(status);
			ICalculator2* ret = static_cast<VTable*>(this->cloopVTable)->createCalculator2(this, status);
			StatusType::checkException(status);
			return ret;
		}

		template <typename StatusType> ICalculator* createBrokenCalculator(StatusType* status)
		{
			StatusType::clearException(status);
			ICalculator* ret = static_cast<VTable*>(this->cloopVTable)->createBrokenCalculator(this, status);
			StatusType::checkException(status);
			return ret;
		}
	};

	class ICalculator : public IDisposable
	{
	public:
		struct VTable : public IDisposable::VTable
		{
			int (CLOOP_CARG *sum)(const ICalculator* self, IStatus* status, int n1, int n2) throw();
			int (CLOOP_CARG *getMemory)(const ICalculator* self) throw();
			void (CLOOP_CARG *setMemory)(ICalculator* self, int n) throw();
			void (CLOOP_CARG *sumAndStore)(ICalculator* self, IStatus* status, int n1, int n2) throw();
		};

	protected:
		ICalculator(DoNotInherit)
			: IDisposable(DoNotInherit())
		{
		}

		~ICalculator()
		{
		}

	public:
		static const unsigned VERSION = 4;

		template <typename StatusType> int sum(StatusType* status, int n1, int n2) const
		{
			StatusType::clearException(status);
			int ret = static_cast<VTable*>(this->cloopVTable)->sum(this, status, n1, n2);
			StatusType::checkException(status);
			return ret;
		}

		int getMemory() const
		{
			if (cloopVTable->version < 3)
			{
				return IStatus::ERROR_1;
			}
			int ret = static_cast<VTable*>(this->cloopVTable)->getMemory(this);
			return ret;
		}

		void setMemory(int n)
		{
			if (cloopVTable->version < 3)
			{
				return;
			}
			static_cast<VTable*>(this->cloopVTable)->setMemory(this, n);
		}

		template <typename StatusType> void sumAndStore(StatusType* status, int n1, int n2)
		{
			if (cloopVTable->version < 4)
			{
				StatusType::setVersionError(status, "ICalculator", cloopVTable->version, 4);
				StatusType::checkException(status);
				return;
			}
			StatusType::clearException(status);
			static_cast<VTable*>(this->cloopVTable)->sumAndStore(this, status, n1, n2);
			StatusType::checkException(status);
		}
	};

	class ICalculator2 : public ICalculator
	{
	public:
		struct VTable : public ICalculator::VTable
		{
			int (CLOOP_CARG *multiply)(const ICalculator2* self, IStatus* status, int n1, int n2) throw();
			void (CLOOP_CARG *copyMemory)(ICalculator2* self, const ICalculator* calculator) throw();
			void (CLOOP_CARG *copyMemory2)(ICalculator2* self, const int* address) throw();
		};

	protected:
		ICalculator2(DoNotInherit)
			: ICalculator(DoNotInherit())
		{
		}

		~ICalculator2()
		{
		}

	public:
		static const unsigned VERSION = 6;

		template <typename StatusType> int multiply(StatusType* status, int n1, int n2) const
		{
			StatusType::clearException(status);
			int ret = static_cast<VTable*>(this->cloopVTable)->multiply(this, status, n1, n2);
			StatusType::checkException(status);
			return ret;
		}

		void copyMemory(const ICalculator* calculator)
		{
			static_cast<VTable*>(this->cloopVTable)->copyMemory(this, calculator);
		}

		void copyMemory2(const int* address)
		{
			if (cloopVTable->version < 6)
			{
				return;
			}
			static_cast<VTable*>(this->cloopVTable)->copyMemory2(this, address);
		}
	};

	// Interfaces implementations

	template <typename Name, typename StatusType, typename Base>
	class IDisposableBaseImpl : public Base
	{
	public:
		typedef IDisposable Declaration;

		IDisposableBaseImpl(DoNotInherit = DoNotInherit())
		{
			static struct VTableImpl : Base::VTable
			{
				VTableImpl()
				{
					this->version = Base::VERSION;
					this->dispose = &Name::cloopdisposeDispatcher;
				}
			} vTable;

			this->cloopVTable = &vTable;
		}

		static void CLOOP_CARG cloopdisposeDispatcher(IDisposable* self) throw()
		{
			try
			{
				static_cast<Name*>(self)->Name::dispose();
			}
			catch (...)
			{
				StatusType::catchException(0);
			}
		}
	};

	template <typename Name, typename StatusType, typename Base = Inherit<IDisposable> >
	class IDisposableImpl : public IDisposableBaseImpl<Name, StatusType, Base>
	{
	protected:
		IDisposableImpl(DoNotInherit = DoNotInherit())
		{
		}

	public:
		virtual ~IDisposableImpl()
		{
		}

		virtual void dispose() = 0;
	};

	template <typename Name, typename StatusType, typename Base>
	class IStatusBaseImpl : public Base
	{
	public:
		typedef IStatus Declaration;

		IStatusBaseImpl(DoNotInherit = DoNotInherit())
		{
			static struct VTableImpl : Base::VTable
			{
				VTableImpl()
				{
					this->version = Base::VERSION;
					this->dispose = &Name::cloopdisposeDispatcher;
					this->getCode = &Name::cloopgetCodeDispatcher;
					this->setCode = &Name::cloopsetCodeDispatcher;
				}
			} vTable;

			this->cloopVTable = &vTable;
		}

		static int CLOOP_CARG cloopgetCodeDispatcher(const IStatus* self) throw()
		{
			try
			{
				return static_cast<const Name*>(self)->Name::getCode();
			}
			catch (...)
			{
				StatusType::catchException(0);
				return static_cast<int>(0);
			}
		}

		static void CLOOP_CARG cloopsetCodeDispatcher(IStatus* self, int code) throw()
		{
			try
			{
				static_cast<Name*>(self)->Name::setCode(code);
			}
			catch (...)
			{
				StatusType::catchException(0);
			}
		}

		static void CLOOP_CARG cloopdisposeDispatcher(IDisposable* self) throw()
		{
			try
			{
				static_cast<Name*>(self)->Name::dispose();
			}
			catch (...)
			{
				StatusType::catchException(0);
			}
		}
	};

	template <typename Name, typename StatusType, typename Base = IDisposableImpl<Name, StatusType, Inherit<IStatus> > >
	class IStatusImpl : public IStatusBaseImpl<Name, StatusType, Base>
	{
	protected:
		IStatusImpl(DoNotInherit = DoNotInherit())
		{
		}

	public:
		virtual ~IStatusImpl()
		{
		}

		virtual int getCode() const = 0;
		virtual void setCode(int code) = 0;
	};

	template <typename Name, typename StatusType, typename Base>
	class IFactoryBaseImpl : public Base
	{
	public:
		typedef IFactory Declaration;

		IFactoryBaseImpl(DoNotInherit = DoNotInherit())
		{
			static struct VTableImpl : Base::VTable
			{
				VTableImpl()
				{
					this->version = Base::VERSION;
					this->dispose = &Name::cloopdisposeDispatcher;
					this->createStatus = &Name::cloopcreateStatusDispatcher;
					this->createCalculator = &Name::cloopcreateCalculatorDispatcher;
					this->createCalculator2 = &Name::cloopcreateCalculator2Dispatcher;
					this->createBrokenCalculator = &Name::cloopcreateBrokenCalculatorDispatcher;
				}
			} vTable;

			this->cloopVTable = &vTable;
		}

		static IStatus* CLOOP_CARG cloopcreateStatusDispatcher(IFactory* self) throw()
		{
			try
			{
				return static_cast<Name*>(self)->Name::createStatus();
			}
			catch (...)
			{
				StatusType::catchException(0);
				return static_cast<IStatus*>(0);
			}
		}

		static ICalculator* CLOOP_CARG cloopcreateCalculatorDispatcher(IFactory* self, IStatus* status) throw()
		{
			StatusType status2(status);

			try
			{
				return static_cast<Name*>(self)->Name::createCalculator(&status2);
			}
			catch (...)
			{
				StatusType::catchException(&status2);
				return static_cast<ICalculator*>(0);
			}
		}

		static ICalculator2* CLOOP_CARG cloopcreateCalculator2Dispatcher(IFactory* self, IStatus* status) throw()
		{
			StatusType status2(status);

			try
			{
				return static_cast<Name*>(self)->Name::createCalculator2(&status2);
			}
			catch (...)
			{
				StatusType::catchException(&status2);
				return static_cast<ICalculator2*>(0);
			}
		}

		static ICalculator* CLOOP_CARG cloopcreateBrokenCalculatorDispatcher(IFactory* self, IStatus* status) throw()
		{
			StatusType status2(status);

			try
			{
				return static_cast<Name*>(self)->Name::createBrokenCalculator(&status2);
			}
			catch (...)
			{
				StatusType::catchException(&status2);
				return static_cast<ICalculator*>(0);
			}
		}

		static void CLOOP_CARG cloopdisposeDispatcher(IDisposable* self) throw()
		{
			try
			{
				static_cast<Name*>(self)->Name::dispose();
			}
			catch (...)
			{
				StatusType::catchException(0);
			}
		}
	};

	template <typename Name, typename StatusType, typename Base = IDisposableImpl<Name, StatusType, Inherit<IFactory> > >
	class IFactoryImpl : public IFactoryBaseImpl<Name, StatusType, Base>
	{
	protected:
		IFactoryImpl(DoNotInherit = DoNotInherit())
		{
		}

	public:
		virtual ~IFactoryImpl()
		{
		}

		virtual IStatus* createStatus() = 0;
		virtual ICalculator* createCalculator(StatusType* status) = 0;
		virtual ICalculator2* createCalculator2(StatusType* status) = 0;
		virtual ICalculator* createBrokenCalculator(StatusType* status) = 0;
	};

	template <typename Name, typename StatusType, typename Base>
	class ICalculatorBaseImpl : public Base
	{
	public:
		typedef ICalculator Declaration;

		ICalculatorBaseImpl(DoNotInherit = DoNotInherit())
		{
			static struct VTableImpl : Base::VTable
			{
				VTableImpl()
				{
					this->version = Base::VERSION;
					this->dispose = &Name::cloopdisposeDispatcher;
					this->sum = &Name::cloopsumDispatcher;
					this->getMemory = &Name::cloopgetMemoryDispatcher;
					this->setMemory = &Name::cloopsetMemoryDispatcher;
					this->sumAndStore = &Name::cloopsumAndStoreDispatcher;
				}
			} vTable;

			this->cloopVTable = &vTable;
		}

		static int CLOOP_CARG cloopsumDispatcher(const ICalculator* self, IStatus* status, int n1, int n2) throw()
		{
			StatusType status2(status);

			try
			{
				return static_cast<const Name*>(self)->Name::sum(&status2, n1, n2);
			}
			catch (...)
			{
				StatusType::catchException(&status2);
				return static_cast<int>(0);
			}
		}

		static int CLOOP_CARG cloopgetMemoryDispatcher(const ICalculator* self) throw()
		{
			try
			{
				return static_cast<const Name*>(self)->Name::getMemory();
			}
			catch (...)
			{
				StatusType::catchException(0);
				return static_cast<int>(0);
			}
		}

		static void CLOOP_CARG cloopsetMemoryDispatcher(ICalculator* self, int n) throw()
		{
			try
			{
				static_cast<Name*>(self)->Name::setMemory(n);
			}
			catch (...)
			{
				StatusType::catchException(0);
			}
		}

		static void CLOOP_CARG cloopsumAndStoreDispatcher(ICalculator* self, IStatus* status, int n1, int n2) throw()
		{
			StatusType status2(status);

			try
			{
				static_cast<Name*>(self)->Name::sumAndStore(&status2, n1, n2);
			}
			catch (...)
			{
				StatusType::catchException(&status2);
			}
		}

		static void CLOOP_CARG cloopdisposeDispatcher(IDisposable* self) throw()
		{
			try
			{
				static_cast<Name*>(self)->Name::dispose();
			}
			catch (...)
			{
				StatusType::catchException(0);
			}
		}
	};

	template <typename Name, typename StatusType, typename Base = IDisposableImpl<Name, StatusType, Inherit<ICalculator> > >
	class ICalculatorImpl : public ICalculatorBaseImpl<Name, StatusType, Base>
	{
	protected:
		ICalculatorImpl(DoNotInherit = DoNotInherit())
		{
		}

	public:
		virtual ~ICalculatorImpl()
		{
		}

		virtual int sum(StatusType* status, int n1, int n2) const = 0;
		virtual int getMemory() const = 0;
		virtual void setMemory(int n) = 0;
		virtual void sumAndStore(StatusType* status, int n1, int n2) = 0;
	};

	template <typename Name, typename StatusType, typename Base>
	class ICalculator2BaseImpl : public Base
	{
	public:
		typedef ICalculator2 Declaration;

		ICalculator2BaseImpl(DoNotInherit = DoNotInherit())
		{
			static struct VTableImpl : Base::VTable
			{
				VTableImpl()
				{
					this->version = Base::VERSION;
					this->dispose = &Name::cloopdisposeDispatcher;
					this->sum = &Name::cloopsumDispatcher;
					this->getMemory = &Name::cloopgetMemoryDispatcher;
					this->setMemory = &Name::cloopsetMemoryDispatcher;
					this->sumAndStore = &Name::cloopsumAndStoreDispatcher;
					this->multiply = &Name::cloopmultiplyDispatcher;
					this->copyMemory = &Name::cloopcopyMemoryDispatcher;
					this->copyMemory2 = &Name::cloopcopyMemory2Dispatcher;
				}
			} vTable;

			this->cloopVTable = &vTable;
		}

		static int CLOOP_CARG cloopmultiplyDispatcher(const ICalculator2* self, IStatus* status, int n1, int n2) throw()
		{
			StatusType status2(status);

			try
			{
				return static_cast<const Name*>(self)->Name::multiply(&status2, n1, n2);
			}
			catch (...)
			{
				StatusType::catchException(&status2);
				return static_cast<int>(0);
			}
		}

		static void CLOOP_CARG cloopcopyMemoryDispatcher(ICalculator2* self, const ICalculator* calculator) throw()
		{
			try
			{
				static_cast<Name*>(self)->Name::copyMemory(calculator);
			}
			catch (...)
			{
				StatusType::catchException(0);
			}
		}

		static void CLOOP_CARG cloopcopyMemory2Dispatcher(ICalculator2* self, const int* address) throw()
		{
			try
			{
				static_cast<Name*>(self)->Name::copyMemory2(address);
			}
			catch (...)
			{
				StatusType::catchException(0);
			}
		}

		static int CLOOP_CARG cloopsumDispatcher(const ICalculator* self, IStatus* status, int n1, int n2) throw()
		{
			StatusType status2(status);

			try
			{
				return static_cast<const Name*>(self)->Name::sum(&status2, n1, n2);
			}
			catch (...)
			{
				StatusType::catchException(&status2);
				return static_cast<int>(0);
			}
		}

		static int CLOOP_CARG cloopgetMemoryDispatcher(const ICalculator* self) throw()
		{
			try
			{
				return static_cast<const Name*>(self)->Name::getMemory();
			}
			catch (...)
			{
				StatusType::catchException(0);
				return static_cast<int>(0);
			}
		}

		static void CLOOP_CARG cloopsetMemoryDispatcher(ICalculator* self, int n) throw()
		{
			try
			{
				static_cast<Name*>(self)->Name::setMemory(n);
			}
			catch (...)
			{
				StatusType::catchException(0);
			}
		}

		static void CLOOP_CARG cloopsumAndStoreDispatcher(ICalculator* self, IStatus* status, int n1, int n2) throw()
		{
			StatusType status2(status);

			try
			{
				static_cast<Name*>(self)->Name::sumAndStore(&status2, n1, n2);
			}
			catch (...)
			{
				StatusType::catchException(&status2);
			}
		}

		static void CLOOP_CARG cloopdisposeDispatcher(IDisposable* self) throw()
		{
			try
			{
				static_cast<Name*>(self)->Name::dispose();
			}
			catch (...)
			{
				StatusType::catchException(0);
			}
		}
	};

	template <typename Name, typename StatusType, typename Base = ICalculatorImpl<Name, StatusType, Inherit<IDisposableImpl<Name, StatusType, Inherit<ICalculator2> > > > >
	class ICalculator2Impl : public ICalculator2BaseImpl<Name, StatusType, Base>
	{
	protected:
		ICalculator2Impl(DoNotInherit = DoNotInherit())
		{
		}

	public:
		virtual ~ICalculator2Impl()
		{
		}

		virtual int multiply(StatusType* status, int n1, int n2) const = 0;
		virtual void copyMemory(const ICalculator* calculator) = 0;
		virtual void copyMemory2(const int* address) = 0;
	};
};


#endif	// CALC_CPP_API_H
