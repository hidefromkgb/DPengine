#ifndef EXEC_WEB_HPP
#define EXEC_WEB_HPP

#include "exec_window.hpp"

// asks the user, then downloads REPO (a directory in the BRAN branch of the
// AUTH/PROJ GitHub project) to DISK if it does not exist, or re-downloads
// the files that differ from REPO if it does; USER is the HTTP user agent
void try_get_from_github(main_window_t &mctl, const char *user,
        const char *auth, const char *proj, const char *bran,
        const char *repo, const char *disk);

#endif // EXEC_WEB_HPP
