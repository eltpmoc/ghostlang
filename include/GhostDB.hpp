#pragma once

#include <string>

int ghost_db_open(const std::string& path);
int ghost_db_exec(int handle, const std::string& sql);
std::string ghost_db_query(int handle, const std::string& sql);
int ghost_db_close(int handle);
int ghost_db_last_id(int handle);
int ghost_db_changes(int handle);
