# filewipe
Secure file wipe utility

This is a utility I used to get experience with memory mapped file performance on both Windows and Linux. It should only be used on physical hard drives. If your storage is an SSD or NVME storage, you'll be wasting a large amount of you usage limits unnecessarily.

The makefile is intended to compile under Linux with Windows mingw64 compiler available. I was also using cross-compiling for the first time.

It works by overwriting the file with different patterns of data that in combination should render the file unrecoverable except by the most proficient of labs. Any mistakes are purely accidental, but use at your own risk.
