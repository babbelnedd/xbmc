#pragma once

/*
*      Copyright (C) 2005-2013 Team XBMC
*      http://www.xbmc.org
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/
#include <boost/scoped_ptr.hpp>

#include <gtest/gtest.h>

#ifdef UNICODE
  typedef CStdStringW        CStdString;
#else
  typedef CStdStringA        CStdString;
#endif

typedef int32_t pid_t;

class WestonTest :
  public ::testing::Test
{
public:

  WestonTest();
  ~WestonTest();
  pid_t Pid();
  const CStdString & TempSocketName();
  
  virtual void SetUp();

private:

  class Private;
  boost::scoped_ptr<Private> priv;
};
