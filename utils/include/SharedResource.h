#pragma once
#include <stdint.h>
#include <vector>


class SharedResource
{
		uint64_t _modify_id;
		uint32_t _modify_counter;
		int _fd;

		SharedResource(const SharedResource&) = delete;

		void GenerateModifyId() noexcept;
		bool Lock(int op, int timeout) noexcept;

	public:
		static bool sEnum(const char *group, std::vector<uint64_t> &ids) noexcept;
		static bool sCleanup(const char *group, uint64_t id) noexcept;

		SharedResource(const char *group, uint64_t id) noexcept;
		~SharedResource();

		bool LockRead(int timeout = -1) noexcept;
		bool LockWrite(int timeout = -1) noexcept;

		void UnlockRead() noexcept;
		void UnlockWrite() noexcept;

		bool IsModified() noexcept;

		struct LockerState
		{
			inline bool Locked() const
			{
				return _locked;
			}

		protected:
			SharedResource &_s;
			bool _locked;

			LockerState(SharedResource &s, bool locked) : _s(s), _locked(locked) {}
		};

		struct Writer : LockerState
		{
			Writer(SharedResource &s, int timeout = -1) : LockerState(s, s.LockWrite(timeout)) { }
			~Writer()
			{
				EnsureUnlocked();
			}

			void EnsureUnlocked()
			{
				if (_locked) {
					_s.UnlockWrite();
					_locked = false;
				}
			}
		};

		struct Reader : LockerState
		{
			Reader(SharedResource &s, int timeout = -1) : LockerState(s, s.LockRead(timeout)) { }
			~Reader()
			{
				EnsureUnlocked();
			}
			void EnsureUnlocked()
			{
				if (_locked) {
					_s.UnlockRead();
					_locked = false;
				}
			}
		};
};
