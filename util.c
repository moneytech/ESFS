/*
  This file is part of ESFS, a FUSE-based filesystem that supports snapshots.
  ESFS is Copyright (C) 2013 Elod Csirmaz
  <http://www.epcsirmaz.com/> <https://github.com/csirmaz>.

  ESFS is based on Big Brother File System (fuse-tutorial)
  Copyright (C) 2012 Joseph J. Pfeiffer, Jr., Ph.D. <pfeiffer@cs.nmsu.edu>,
  and was forked from it on 21 August 2013.
  Big Brother File System can be distributed under the terms of
  the GNU GPLv3. See the file COPYING.
  See also <http://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/>.

  Big Brother File System was derived from function prototypes found in
  /usr/include/fuse/fuse.h
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  fuse.h is licensed under the LGPLv2.

  ESFS is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.

  ESFS is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
  details.

  You should have received a copy of the GNU General Public License along
  with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * NOTE: A Perl script is used to replace $ with esfs_ and $$ with ESFS_
 * in this file. To write $, use \$.
 */

// This file contains low-level tools.

/* USEFUL
 * strerror(errno)
 */


// Sleep: needs struct timespec delay
#define $$SLEEP pselect(0, NULL, NULL, NULL, &delay, NULL);


// Extract and cast fsdata
#define $$FSDATA ((struct $fsdata_t *) fuse_get_context()->private_data )


// Define, extract and cast fsdata
#define $$DFSDATA struct $fsdata_t *fsdata; fsdata = $$FSDATA;


// Checks if (virtual) path is in the snapshot space
#define $$_IS_PATH_IN_SN(path) (unlikely(strncmp(path, $$SNDIR, $$SNDIR_LEN) == 0 && (path[$$SNDIR_LEN] == $$DIRSEPCH || path[$$SNDIR_LEN] == '\0')))


// Use this when the command writes - we don't allow that in the snapshot dir, only in the main space.
// Uses path; Defines fpath, fsdata
#define $$IF_PATH_MAIN_ONLY \
   char fpath[$$PATH_MAX]; \
   $$DFSDATA \
   if($$_IS_PATH_IN_SN(path)){ return -EACCES; } \
   if($map_path(fpath, path, fsdata) != 0){ return -ENAMETOOLONG; }


// Use this when there are two paths, path and newpath, and the command writes.
// Uses path; Defines fpath, fnewpath, fsdata
#define $$IF_MULTI_PATHS_MAIN_ONLY \
   char fpath[$$PATH_MAX]; \
   char fnewpath[$$PATH_MAX]; \
   $$DFSDATA \
   if($$_IS_PATH_IN_SN(path)){ return -EACCES; } \
   if($map_path(fpath, path, fsdata) != 0){ return -ENAMETOOLONG; } \
   if($$_IS_PATH_IN_SN(newpath)){ return -EACCES; } \
   if($map_path(fnewpath, newpath, fsdata) != 0){ return -ENAMETOOLONG; }


// Use these when there are different things to do in the two spaces
// Uses path; Defines fpath, fsdata, snpath, ret
// First branch: in snapshot space. fpath contains the mapped path; snpath the decomposed paths.
// DO NOT RETURN FROM THE FOLLOWING BLOCK! SET 'ret' INSTEAD!
// ! For performance, we only allocate memory for snpath when needed, but because of this,
// one cannot return in the SN branch.
#define $$IF_PATH_SN \
   char fpath[$$PATH_MAX]; \
   struct $snpath_t *snpath; \
   int ret = -EBADE; \
   $$DFSDATA \
   if($map_path(fpath, path, fsdata) != 0){ return -ENAMETOOLONG; } \
   if($$_IS_PATH_IN_SN(path)){ \
      snpath = malloc(sizeof(struct $snpath_t)); \
      if(snpath == NULL){ return -ENOMEM; } \
      $decompose_sn_path(snpath, path);

// Second branch: in main space. fpath contains the mapped path.
#define $$ELIF_PATH_MAIN \
      free(snpath); \
      return ret; \
   }

// Uses ret
#define $$FI_PATH \
   return -EFAULT;


// Use this to allow the operation everywhere.
// TODO Later, replace this with something that hides .hid files
// and deals with .dat extensions on non-directories.
// Uses path; Defines fpath, fsdata
#define $$ALL_PATHS \
   char fpath[$$PATH_MAX]; \
   $$DFSDATA \
   switch($map_path(fpath, path, fsdata)){ \
      case -ENAMETOOLONG : return -ENAMETOOLONG; \
   }


// Adds a suffix to a path (STANDALONE - ALWAYS RETURNS!)
// Returns:
// 0 - success
// -ENAMETOOLONG - if the new path is too long
#define $$ADDNSUFFIX_RET(newpath, oldpdath, fix, fixlen) \
   if(likely(strlen(oldpath) < $$PATH_MAX - fixlen)){ \
      strcpy(newpath, oldpath); \
      strncat(newpath, fix, fixlen); \
      return 0; \
   } \
   return -ENAMETOOLONG;


// Adds a prefix to a path (MAY RETURN)
// Needs $$PATH_LEN_T plen
// Returns:
// -ENAMETOOLONG - if the new path is too long
#define $$ADDNPREFIX_CONT(newpath, oldpath, fix, fixlen) \
   plen = $$PATH_MAX - fixlen; \
   if(unlikely(strlen(oldpath) >= plen)){ \
      return -ENAMETOOLONG; \
   } \
   strcpy(newpath, fix); \
   strncat(newpath, oldpath, plen); \


// Adds a prefix to a path (STANDALONE - ALWAYS RETURNS!)
// Returns:
// 0 - success
// -ENAMETOOLONG - if the new path is too long
#define $$ADDNPREFIX_RET(newpath, oldpath, fix, fixlen) \
   $$PATH_LEN_T plen; \
   $$ADDNPREFIX_CONT(newpath, oldpath, fix, fixlen) \
   return 0;


// Adds a prefix and two suffixes to a path (MAY RETURN)
// Needs $$PATH_LEN_T plen
// Returns -ENAMETOOLONG - if the new path is too long
// Otherwise CONTINUES
#define $$ADDNPSFIX_CONT(newpath1, newpath2, oldpath, prefix, prefixlen, suffix1, suffix2, suffixlen) \
   plen = $$PATH_MAX - prefixlen - suffixlen; \
   if(unlikely(strlen(oldpath) >= plen)){ \
      return -ENAMETOOLONG; \
   } \
   strcpy(newpath1, prefix); \
   strncat(newpath1, oldpath, plen); \
   strcpy(newpath2, newpath1); \
   strncat(newpath1, suffix1, suffixlen); \
   strncat(newpath2, suffix2, suffixlen);


// Adds the "hidden" suffix to a path
// Returns:
// 0 - success
// -ENAMETOOLONG - if the new path is too long
static inline int $get_hid_path(char *newpath, const char *oldpath)
{
   $$ADDNSUFFIX_RET(newpath, oldpath, $$EXT_HID, $$EXT_LEN)
}


// Maps virtual path into real path
// Puts the mapped path in fpath and returns
// 0 - if the path is in the main space
// -ENAMETOOLONG - if the mapped path is too long
static inline int $map_path(char *fpath, const char *path, const struct $fsdata_t *fsdata)
{
   $$ADDNPREFIX_RET(fpath, path, fsdata->rootdir, fsdata->rootdir_len)
}


// Breaks up a VIRTUAL path in the snapshots space into a struct $snpath_t
// "/snapshots/ID/dir/dir/file"
// Returns snpath->is_there:
// 0 - if the string is "/snapshots" or "/snapshots?" (from $$_IS_PATH_IN_SN we'll know that ?='/') or "/snapshots//"
// 1 - if the string is "/snapshots/ID($|/)"
// 2 - if the string is "/snapshots/ID/..."
static int $decompose_sn_path(struct $snpath_t *snpath, const char *path)
{
   $$PATH_LEN_T len;
   $$PATH_LEN_T idlen;
   const char *idstart;
   const char *nextslash;

   len = strlen(path);
   // we know the string starts with "/snapshots"
   if(len <= $$SNDIR_LEN + 1){ // the string is "/snapshots" or "/snapshots?"
      snpath->is_there = 0;
      return 0;
   }

   idstart = $$SNDIR_LEN + path; // points to the slash at the end of "/snapshots/"
   nextslash = strchr(idstart + 1, $$DIRSEPCH);

   if(nextslash == NULL){ // there's no next '/', so the string must be "/snapshots/ID"
      snpath->is_there = 1;
      strcpy(snpath->id, idstart);
      return 1;
   }

   if(nextslash == idstart + 1){ // the string must be "/snapshots//"
      snpath->is_there = 0;
      return 0;
   }

   if(nextslash == path + len - 1){ // the next slash the last character, so "/snapshots/ID/"
      snpath->is_there = 1;
      idlen = len - $$SNDIR_LEN - 1;
      strncpy(snpath->id, idstart, idlen);
      snpath->id[idlen] = '\0';
      return 1;
   }

   // otherwise we've got "/snapshots/ID/..."
   snpath->is_there = 2;
   idlen = nextslash - idstart;
   strncpy(snpath->id, idstart, idlen);
   snpath->id[idlen] = '\0';
   strcpy(snpath->inpath, nextslash);
   return 0;
}


// Implementation of rm -r
// $recursive_remove returns 0 or -errno.
// The helper function, $_univ_rm returns 0 or errno.
int $_univ_rm(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
   if(typeflag == FTW_DP || typeflag == FTW_D || typeflag == FTW_DNR){
      if(rmdir(fpath) != 0){ return errno; }
      return 0;
   }

   if(unlink(fpath) != 0){ return errno; }
   return 0;
}

static inline int $recursive_remove(const char *path)
{
   int ret;
   ret = nftw(path, $_univ_rm, $$RECURSIVE_RM_FDS, FTW_DEPTH | FTW_PHYS);
   if(ret >= 0){ return -ret; } // success or errno
   return ENOMEM; // generic error encountered by nftw
}


// Read a snapshot dir path from a file
// Returns:
// length of path - on success
// 0 - if the file does not exist
// -errno - on other failure
static int $get_sndir_from_file(const struct $fsdata_t *fsdata, char buf[$$PATH_MAX], const char *filepath)
{
   int fd;
   int ret;

   if((fd = open(filepath, O_RDONLY)) == -1){
      fd = errno;
      if(fd == ENOENT){
         return 0;
      }
      $dlogi("get_path_from_file: opening %s failed with %d = %s\n", filepath, fd, strerror(fd));
      return -fd;
   }

   ret = pread(fd, buf, $$PATH_MAX, 0);
   if(ret == -1){
      ret = errno;
      $dlogi("get_path_from_file: reading from %s failed with %d = %s\n", filepath, ret, strerror(ret));
      return -ret;
   }
   if(ret == 0 || buf[ret] != '\0'){
      $dlogi("get_path_from_file: reading from %s returned 0 bytes or string is not 0-terminated.\n", filepath);
      return -EIO;
   }

   close(fd);

   // TODO check if the directory really exists before returning success

   return (ret - 1);
}


// Check consistency of constants
// Returns 0 on success, -1 on failure
int $check_params(void){
   // Check string lengths
   if(strlen($$SNDIR) != $$SNDIR_LEN){ return -51; }
   if(strlen($$EXT_DAT) != $$EXT_LEN){ return -52; }
   if(strlen($$EXT_HID) != $$EXT_LEN){ return -53; }
   if(strlen($$DIRSEP) != 1){ return -54; }

   // Check if the block and block pointer sizes make it possible to store files off_t large.
   // The max file size + 1 on the system is 2^( sizeof(off_t)*8 ) bytes.
   // The max file size + 1 in ESFS is $$BL_S * 2^($$BLP_S*8) = 2^( log($$BL_S) + $$BLP_S*8 ) bytes.
   if(sizeof($$BLP_T) != $$BLP_S){ return -10; }
   if( sizeof(off_t) * 8.0 > ( $$BL_SLOG + ((double)$$BLP_S)*8.0) ){ return -11; }
   if( (1 << $$BL_SLOG) != $$BL_S ){ return -12; }

   return 0;
}

