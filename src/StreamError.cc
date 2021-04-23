/*************************************************************************
* This is error and debug message handling of StreamDevice.
* Please see ../docs/ for detailed documentation.
*
* (C) 2005 Dirk Zimoch (dirk.zimoch@psi.ch)
*
* This file is part of StreamDevice.
*
* StreamDevice is free software: You can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published
* by the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* StreamDevice is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with StreamDevice. If not, see https://www.gnu.org/licenses/.
*************************************************************************/

#include "StreamError.h"
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif /* _WIN32 */
#include <string.h>
#include <time.h>
#include <stdio.h>

int streamDebug = 0;
int streamError = 1;
FILE *StreamDebugFile = NULL;

/*0: disable timestamps on stream messages (both debug and error)*/
int streamMsgTimeStamped = 1; 

#ifndef va_copy
#ifdef __va_copy
#define va_copy __va_copy
#endif
#endif

#ifdef _WIN32
#define localtime_r(timet,tm) localtime_s(tm,timet)

/* this may not be defined if using older Windows SDKs */
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

/* Enable ANSI color support in Windows console */
static bool win_console_init() {
    HANDLE hCons[] = { GetStdHandle(STD_ERROR_HANDLE),
                       GetStdHandle(STD_OUTPUT_HANDLE) };
    for(int i=0; i < sizeof(hCons) / sizeof(HANDLE); ++i)
    {
        DWORD dwMode = 0;
        if (hCons[i] == NULL ||
            hCons[i] == INVALID_HANDLE_VALUE ||
            !GetConsoleMode(hCons[i], &dwMode))
        {
            return false;
        }
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (!SetConsoleMode(hCons[i], dwMode))
        {
            return false;
        }
    }
    return true;
}

/* do isatty() call second as always want to run win_console_init() */
int streamDebugColored = win_console_init() && _isatty(_fileno(stdout));

#else

int streamDebugColored = isatty(fileno(stdout));

#endif /* _WIN32 */

/* You can globally change the printTimestamp function
   by setting the StreamPrintTimestampFunction variable
   to your own function.
*/
static void printTimestamp(char* buffer, size_t size)
{
    time_t t;
    struct tm tm;
    time(&t);
    localtime_r(&t, &tm);
    strftime(buffer, size, "%Y/%m/%d %H:%M:%S", &tm);
}

void (*StreamPrintTimestampFunction)(char* buffer, size_t size) = printTimestamp;

void StreamError(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    StreamVError(0, NULL, fmt, args);
    va_end(args);
}

void StreamError(int line, const char* file, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    StreamVError(line, file, fmt, args);
    va_end(args);
}

void StreamVError(int line, const char* file, const char* fmt, va_list args)
{
    char timestamp[40];
    if (!(streamError || streamDebug)) return; // Error logging disabled
    strcpy(timestamp, "");
    if (streamMsgTimeStamped != 0)
    {
        StreamPrintTimestampFunction(timestamp, 40);
    }
#ifdef va_copy
    if (StreamDebugFile)
    {
        va_list args2;
        va_copy(args2, args);
        fprintf(StreamDebugFile, "%s ", timestamp);
        vfprintf(StreamDebugFile, fmt, args2);
        fflush(StreamDebugFile);
        va_end(args2);
    }
#endif
    fprintf(stderr, "%s%s ", ansiEscape(ANSI_RED_BOLD), timestamp);
    if (file)
    {
        fprintf(stderr, "%s line %d: ", file, line);
    }
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "%s", ansiEscape(ANSI_RESET));
}

int StreamDebugClass::
print(const char* fmt, ...)
{
    va_list args;
    char timestamp[40];
    StreamPrintTimestampFunction(timestamp, 40);
    va_start(args, fmt);
    const char* f = strrchr(file, '/');
    if (f) f++; else f = file;
    FILE* fp = StreamDebugFile ? StreamDebugFile : stderr;
    if (streamMsgTimeStamped != 0) 
    {
        fprintf(fp, "%s ", timestamp);
    }
    fprintf(fp, "%s:%d: ", f, line);
    vfprintf(fp, fmt, args);
    fflush(fp);
    va_end(args);
    return 1;
}

/**
 * Return an ANSI escape code if coloured debug output is enabled
 */
const char* ansiEscape(AnsiMode mode)
{
    static const char* AnsiEscapes[] = { "\033[7m", "\033[27m", "\033[47m",
                                         "\033[0m", "\033[31;1m" };
    return streamDebugColored ? AnsiEscapes[mode] : "";
}
