/*
 * filewipe.c - Secure overwrite and deletion of a file
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#ifdef WINDOWS
#include  <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif


#define WIPE_NULL_CHAR  0x00  // 00000000
#define WIPE_MAX_CHAR   0xff  // 11111111
#define WIPE_PTRN1_CHAR 0x55  // 01010101
#define WIPE_PTRN2_CHAR 0xaa  // 10101010
#define WIPE_PTRN3_CHAR 0x3a  // 00111010
#define WIPE_PTRN4_CHAR 0xc5  // 11000101
#define MSG_BANNER      "AB Data Services"
#define MSG_VERSION     "1.3"

#ifdef WINDOWS
#define MSG_MODE   MB_OK | MB_ICONERROR
#endif

#define handle_error( msg ) \
  do { char cMsg[ 2048 ] = { 0 }; snprintf(cMsg, 2048, msg, errno);  \
    perror( cMsg ); exit( EXIT_FAILURE ); } while( 0 )


void doOSFileWipe( char *name );

int main( int argc, char *argv[] )  {
  int   retVal = 0;
  int   numFiles = argc - 1;
  int   i;
  int   memSize = 10485760;
  void *memblk = NULL;
  char  outMsg[ 2048 ] = { 0 };

  fprintf( stdout, "%s: Secure file overwrite and delete utility v%s\n",
           argv[ 0 ], MSG_VERSION );
  fprintf( stdout, "Copyright (c)2018-2020, %s, All rights "
          "reserved worldwide.\n\n", MSG_BANNER );
  if ( argc == 1 )  {
    snprintf( outMsg, 2048,
             "usage: %s <file1> [<file2> [<file3>[...<fileN>]]]\n",
             argv[ 0 ] );
    fprintf( stderr, outMsg );
    exit(1);
  }
  if ( ( memblk = malloc( memSize ) ) != NULL )  {
    memset( memblk, 0, memSize );
    free( memblk );
    for (i = 0; i < numFiles; i++) {
      doOSFileWipe(argv[i + 1]);
    }
  }
  else  {
    snprintf( outMsg, sizeof(outMsg), "malloc() of %d size failed. errno=%d",
              memSize, errno );
    handle_error( outMsg );
  }
  return retVal;
}


#ifdef WINDOWS
void DisplayError(char *errTxt) {
  char  errBuff[1024] = { 0 };
  DWORD lastErr       = GetLastError();

  FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, lastErr,
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errBuff,
                 sizeof(errBuff), NULL);
  strcat(errBuff, ": ");
  strcat(errBuff, errTxt);
  MessageBox(NULL, errBuff, MSG_BANNER, MSG_MODE);
  exit(lastErr);
}

void DisplayErrorMsg(void) {
  char errBuff[1024] = { 0 };

  strcpy(errBuff, "File too large to buffer");
  strcat(errBuff, ": GetFileSize()");
  MessageBox(NULL, errBuff, MSG_BANNER, MSG_MODE);
  exit(1);
}

void winFileWipe(char *fName) {
  LONG   fSize         = 0;
  char    outMsg[2048] = { 0 },
         *ptr          = NULL;
  HANDLE hFile         = NULL;
  LPVOID lpvMem        = NULL;
  HANDLE hMapObject    = NULL;  // handle to file mapping
  BY_HANDLE_FILE_INFORMATION fileInfo;

  /*
   * Open file exclusively
   */
  if ( ( hFile = CreateFile( fName, GENERIC_READ | GENERIC_WRITE, 0, NULL,
               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL ) )
     == INVALID_HANDLE_VALUE)
  DisplayError( "CreateFile()");
  /*
   * Map the file into memory
   */
  if ((hMapObject = CreateFileMapping(
                                      hFile, // use open file
                                      NULL,  // no security attributes
                                      PAGE_READWRITE, // read/write access
                                      0, 0,           // size of file
                                      NULL)           // no name
        ) == NULL)
    DisplayError("CreateFileMapping()");
  snprintf(outMsg, sizeof(outMsg), "Opened |%s|...\r", fName);
  fprintf(stdout, outMsg);
  fflush(stdout);
  outMsg[strlen(outMsg) - 1] = ' ';
  ptr = strdup(outMsg);
  /*
   * Get a pointer to the memory mapped file
   */
  if ( ( lpvMem = MapViewOfFile( hMapObject,     // object to map view of
                                FILE_MAP_WRITE, // read/write access
                                0, 0,            // high/low offset:
                                0)    // default: map entire file
       ) == NULL)
    DisplayError("MapViewOfFile()");
  /*
   * Retrieve the file size
   */
  if (GetFileInformationByHandle(hFile, &fileInfo)) {
    if (fileInfo.nFileSizeHigh > 0) {
       /*
        * report too large err
        */
       DisplayErrorMsg();
    }
    fSize = fileInfo.nFileSizeLow;
  }
  snprintf(outMsg, 2048, "%swiping file...\r",
           ptr);
  fprintf(stdout, outMsg);
  fflush(stdout);
  outMsg[strlen(outMsg) - 1] = ' ';
  free(ptr);
  ptr = strdup(outMsg);
  /*
   * Write 0xff to all bytes of file
   */
  memset(lpvMem, WIPE_MAX_CHAR, fSize);
  if (!FlushViewOfFile(lpvMem, 0)) {
    /*
     *
     */
    DisplayError("FlushViewOfFile() 0xff");
  }
  if (!FlushFileBuffers(hFile)) {
    /*
     *
     */
    DisplayError("FlushFileBuffers() 0xff");
  }
  /*
   * Write 0x00 to all bytes of file
   */
  memset(lpvMem, WIPE_NULL_CHAR, fSize);
  if (!FlushViewOfFile(lpvMem, 0)) {
    /*
     *
     */
    DisplayError("FlushViewOfFile() 0x0");
  }
  if (!FlushFileBuffers(hFile)) {
    /*
     *
     */
    DisplayError("FlushFileBuffers() 0x0");
  }
  /*
   * Write 0x55 to all bytes of file
   */
  memset(lpvMem, WIPE_PTRN1_CHAR, fSize);
  if (!FlushViewOfFile(lpvMem, 0)) {
    /*
     *
     */
    DisplayError("FlushViewOfFile() P1");
  }
  if (!FlushFileBuffers(hFile)) {
    /*
     *
     */
    DisplayError("FlushFileBuffers() P1");
  }
  /*
   * Write 0xaa to all bytes of file
   */
  memset(lpvMem, WIPE_PTRN2_CHAR, fSize);
  if (!FlushViewOfFile(lpvMem, 0)) {
    /*
     *
     */
    DisplayError("FlushViewOfFile() P2");
  }
  if (!FlushFileBuffers(hFile)) {
    /*
     *
     */
    DisplayError("FlushFileBuffers() P2");
  }
  /*
   * Write 0x3a to all bytes of file
   */
  memset(lpvMem, WIPE_PTRN3_CHAR, fSize);
  if (!FlushViewOfFile(lpvMem, 0)) {
    /*
     *
     */
    DisplayError("FlushViewOfFile() P3");
  }
  if (!FlushFileBuffers(hFile)) {
    /*
     *
     */
    DisplayError("FlushFileBuffers() P3");
  }
  /*
   * Write 0xc5 to all bytes of file
   */
  memset(lpvMem, WIPE_PTRN4_CHAR, fSize);
  if (!FlushViewOfFile(lpvMem, 0)) {
    /*
     *
     */
    DisplayError("FlushViewOfFile() P4");
  }
  if (!FlushFileBuffers(hFile)) {
    /*
     *
     */
    DisplayError("FlushFileBuffers() P4");
  }
  /*
   * Write 0x00 to all bytes of file
   */
  memset(lpvMem, WIPE_NULL_CHAR, fSize);
  if (!FlushViewOfFile(lpvMem, 0)) {
    /*
     *
     */
    DisplayError("FlushViewOfFile() 0x0 2");
  }
  if (!FlushFileBuffers(hFile)) {
    /*
     *
     */
    DisplayError("FlushFileBuffers() 0x0 2");
  }
  /*
   * Close all entry points to file
   */
  if (!UnmapViewOfFile(lpvMem)) {
    DisplayError("UnmapViewOfFile()");
  }
  if (!CloseHandle(hMapObject)) {
    DisplayError("CloseHandle(hMapObject)");
  }
  snprintf(outMsg, 2048, "%sunmapped...\r", ptr);
  fprintf(stdout, outMsg);
  fflush(stdout);
  outMsg[strlen(outMsg) - 1] = ' ';
  free(ptr);
  ptr = strdup(outMsg);
  if (!CloseHandle(hFile)) {
    DisplayError("CloseHandle(hFile)");
  }
  snprintf(outMsg, 2048, "%sclosed...\r", ptr);
  fprintf(stdout, outMsg);
  free(ptr);
  outMsg[strlen(outMsg) - 1] = ' ';
  ptr = strdup(outMsg);
  /*
   * Delete the file
   */
  if (!DeleteFileA(fName)) {
    DisplayError("DeleteFileA()");
  }
  snprintf(outMsg, 2048, "%sremoved.\n", ptr);
  fprintf(stdout, outMsg);
  fflush(stdout);
  free(ptr);
}

#else  /* ~WINDOWS */


void unixFileWipe(char *fName) {
  char         outMsg[2048] = { 0 },
              *ptr          = NULL;
  long         fileNo       = -1;
  off_t        offset       = 0;
  int          fSize        = 0;
  int          memSize      = 10485760; /* 10MB work size */
  struct flock lockInfo;
  const  int   fileMode     =
                 ( O_RDWR | O_EXCL | __O_DIRECT | O_NOFOLLOW | O_SYNC );

  if ((fileNo = open(fName, fileMode)) != -1) {
    snprintf(outMsg, sizeof(outMsg), "Opened |%s|...\r", fName );
    fprintf(stdout, outMsg);
    fflush(stdout);
    outMsg[strlen(outMsg) - 1] = ' ';
    ptr = strdup(outMsg);

    if ((offset = lseek(fileNo, 0, SEEK_END)) != -1) {
      fSize = offset;
      if (fSize == 0) {
        errno = 9;
        snprintf(outMsg, sizeof(outMsg),
                 "Empty file not supported |%s| errno=%d", fName);
        handle_error(outMsg);
      }
      if ((offset = lseek(fileNo, 0,
                          SEEK_SET)) != -1) {
        void *mapBlk = NULL;

        if ((mapBlk = mmap(NULL, fSize,
                           PROT_READ | PROT_WRITE,
                           MAP_SHARED /*| MAP_LOCKED*/,
                           fileNo, 0)
            ) != (void*)-1) {

          snprintf(outMsg, 2048, "%swiping file...\r", ptr);
          fprintf(stdout, outMsg);
          fflush(stdout);
          outMsg[strlen(outMsg) - 1] = ' ';
          free(ptr);
          ptr = strdup(outMsg);
          memSize = fSize;
          memset(mapBlk, WIPE_MAX_CHAR, memSize);
          if (msync(mapBlk, memSize, MS_SYNC | MS_INVALIDATE) == -1)
            handle_error("0xff flush failed: %d");

          memset(mapBlk, WIPE_NULL_CHAR, memSize);
          if (msync(mapBlk, memSize, MS_SYNC | MS_INVALIDATE) == -1)
            handle_error("Zero flush failed: %d");

          memset(mapBlk, WIPE_PTRN1_CHAR, memSize);
          if (msync(mapBlk, memSize, MS_SYNC | MS_INVALIDATE) == -1)
            handle_error("0x55 flush failed: %d");

          memset(mapBlk, WIPE_PTRN2_CHAR, memSize);
          if (msync(mapBlk, memSize, MS_SYNC | MS_INVALIDATE) == -1)
            handle_error("0xaa flush failed: %d");

          memset(mapBlk, WIPE_PTRN3_CHAR, memSize);
          if (msync(mapBlk, memSize, MS_SYNC | MS_INVALIDATE) == -1)
            handle_error("0x3a flush failed: %d");

          memset(mapBlk, WIPE_PTRN4_CHAR, memSize);
          if (msync(mapBlk, memSize, MS_SYNC | MS_INVALIDATE) == -1)
            handle_error("0xc5 flush failed: %d");

          memset(mapBlk, WIPE_NULL_CHAR, memSize);
          if (msync(mapBlk, memSize, MS_SYNC | MS_INVALIDATE) == -1)
            handle_error("Zero flush #2 failed: %d");

          if (munmap(mapBlk, memSize) != 0)
            handle_error("munmap() failed: %d");
          snprintf(outMsg, 2048, "%sunmapped...\r", ptr);
          fprintf(stdout, outMsg);
          fflush(stdout);
          outMsg[strlen(outMsg) - 1] = ' ';
          free(ptr);
          ptr = strdup(outMsg);
        }
        else handle_error("mmap() failed: %d");
      }
      else {
        char msg[2048] = { 0 };
        snprintf(msg, 2048,
                 "lseek to beginning failed: %s, %d\n",
                 fName);
        handle_error(msg);
      }
    }
    else {
      char msg[2048] = { 0 };
      snprintf(msg, 2048,
               "lseek for size failed: |%s|, errno = %d",
               fName, errno);
      handle_error(msg);
    }

    if (remove(fName) != 0) {
      char msg[2048] = { 0 };
      snprintf(msg, 2048, "Error removing file: |%s|, errno=%d",
               fName);
      handle_error(msg);
    }
    snprintf(outMsg, 2048, "%sremoved...\r", ptr);
    fprintf(stdout, outMsg);
    fflush(stdout);
    outMsg[strlen(outMsg) - 1] = ' ';
    free(ptr);
    ptr = strdup(outMsg);
    if (close(fileNo) == -1) {
      char msg[2048] = { 0 };
      snprintf(msg, 2048, "Error closing file: |%s|, errno=%d",
               fName, errno);
      handle_error(msg);
    }
    snprintf(outMsg, 2048, "%sclosed.\n", ptr);
    fprintf(stdout, outMsg);
    free(ptr);
    memset(outMsg, 0, sizeof(outMsg));
  }
  else {
    char msg[2048] = { 0 };
    snprintf(msg, 2048, "Open failed on file: |%s|, errno=%d",
             fName, errno);
    handle_error(msg);
  }
}

#endif  /* WINDOWS */


void doOSFileWipe(char *fName) {
#ifdef WINDOWS
  winFileWipe( fName );
#else  /* ~WINDOWS */
  unixFileWipe( fName );
#endif
}

/* filewipe.c */
