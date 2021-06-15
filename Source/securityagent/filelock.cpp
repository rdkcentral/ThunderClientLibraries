#include "filelock.h"

#include <sys/file.h>
#include <unistd.h>

FileLock::Lock::Lock(FileLock* sem)
    : ptr(sem)
    , locked(false) {
    locked = ptr->Wait();
}

FileLock::Lock::~Lock() {
    if (locked) {
        ptr->Unlock();
    }
}

FileLock::FileLock(const char* name) {
    fd_lock = open(name, O_CREAT, S_IRUSR | S_IWUSR);
}

FileLock::~FileLock() {
    close(fd_lock);
}

bool FileLock::Wait() {
    return (flock(fd_lock, LOCK_EX) == 0);
}

bool FileLock::Unlock() {
    return (flock(fd_lock, LOCK_UN) == 0);
}
