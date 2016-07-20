/***************************************************************************************************
 * Copyright (c) 2016, Imagination Technologies Limited and/or its affiliated group companies
 * and/or licensors
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions
 *    and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to
 *    endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file log.h
 * @brief Header file for logging.
 */

#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <string.h>
#include <time.h>

//! \{
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define LOG_FATAL    (1)
#define LOG_ERR      (2)
#define LOG_WARN     (3)
#define LOG_INFO     (4)
#define LOG_DBG      (5)
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define TIME_BUFFER_SIZE  (32)
//! \}

/** Macro for printing logging message with current time, function and line no. */
#define DEBUG_PRINT                                                           \
    do {                                                                      \
        time_t currentTime = time(NULL);                                      \
        char buffer[TIME_BUFFER_SIZE] = {0};                                  \
        strftime(buffer, TIME_BUFFER_SIZE, "%x %X", localtime(&currentTime)); \
        fprintf(g_debugStream,"[%s] ", buffer);                                 \
        fprintf(g_debugStream, ANSI_COLOR_YELLOW);                              \
        fprintf(g_debugStream,"%s:%d: ", __FILENAME__, __LINE__);               \
        fprintf(g_debugStream, ANSI_COLOR_RESET);                               \
    } while (0)

/** Macro for logging message at the specified level. */
#define LOG(level, ...)                                       \
    do {                                                      \
        if (level <= g_debugLevel)                              \
        {                                                     \
            if (g_debugStream == NULL)                          \
                g_debugStream = stdout;                         \
            fprintf(g_debugStream, "\n");                       \
            if (g_debugLevel == LOG_DBG)                        \
            {                                                 \
                DEBUG_PRINT;                                  \
            }                                                 \
            switch (level)                                    \
            {                                                 \
                case LOG_ERR:                                 \
                    fprintf(g_debugStream, ANSI_COLOR_RED);     \
                    break;                                    \
                case LOG_INFO:                                \
                    fprintf(g_debugStream, ANSI_COLOR_CYAN);    \
                    break;                                    \
                default:                                      \
                    break;                                    \
            }                                                 \
            fprintf(g_debugStream, __VA_ARGS__);                \
            fprintf(g_debugStream, ANSI_COLOR_RESET);           \
            fprintf(g_debugStream, "\n");                       \
            fflush(g_debugStream);                              \
        }                                                     \
    } while (0)

/** Output stream to dump logs. */
extern FILE *g_debugStream;
/** Debug level for logs. */
extern int g_debugLevel;


#endif  /* LOG_H */
