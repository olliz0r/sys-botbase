/*
 * Copyright (c) 2020 sys-botbase
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 3, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parseArgs(char *origargstr, int (*callback)(int, char **)) {
  char *argstr = malloc(strlen(origargstr) + 1);
  memcpy(argstr, origargstr, strlen(origargstr) + 1);
  int argc = 0;

  char *tmpstr = malloc(strlen(argstr) + 1);
  memcpy(tmpstr, argstr, strlen(argstr) + 1);
  for (char *p = strtok(tmpstr, " \r\n"); p != NULL;
       p = strtok(NULL, " \r\n")) {
    argc++;
  }
  free(tmpstr);

  if (argc == 0) {
    return (*callback)(argc, NULL);
  }

  char **argv = malloc(sizeof(char *) * argc);
  int i = 0;
  for (char *p = strtok(argstr, " \r\n"); p != NULL; p = strtok(NULL, " \r\n"))
    argv[i++] = p;

  int ret = (*callback)(argc, argv);
  free(argv);
  free(argstr);
  return ret;
}