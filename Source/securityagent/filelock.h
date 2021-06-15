#ifndef FILE_LOCK_H
#define FILE_LOCK_H

class FileLock {
public:
    class Lock {
    public:
        Lock() = delete;
        Lock(const Lock &) = delete;
        Lock &operator=(const Lock &) = delete;
        Lock(FileLock *sem);
        ~Lock();
    private:
        FileLock *ptr;
        bool locked;
    };

    FileLock() = delete;
    FileLock(const FileLock &) = delete;
    FileLock &operator=(const FileLock &) = delete;
    FileLock(const char* name);
    ~FileLock();
    bool Wait();
    bool Unlock();

private:
    int fd_lock;
};

#endif //FILE_LOCK_H
