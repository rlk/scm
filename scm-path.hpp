// Copyright (C) 2011-2013 Robert Kooima
//
// LIBSCM is free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITH-
// OUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.

#ifndef SCM_PATH_HPP
#define SCM_PATH_HPP

#include <string>
#include <list>

//------------------------------------------------------------------------------

class scm_path
{
public:

	scm_path();

	std::string search(const std::string&);

	void push(const std::string&);
	void pop();

private:

	std::list<std::string> directories;
};

//------------------------------------------------------------------------------

#endif