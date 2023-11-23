int _close(int fd) { return 1; }
int _fstat(int fd) { return 1; }
int _getpid(int fd) { return 1; }
int _isatty(int fd) { return 1; }
int _kill(int fd) { return 1; }
int _read(int fd, char* ptr, int len) { return 0; }
int _lseek(int fd) { return 1; }
