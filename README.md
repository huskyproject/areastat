# AreaStat
[![Build Status](https://travis-ci.org/huskyproject/areastat.svg?branch=master)](https://travis-ci.org/huskyproject/areastat)
[![Build status](https://ci.appveyor.com/api/projects/status/9pq58llqhw5ijh46/branch/master?svg=true)](https://ci.appveyor.com/project/dukelsky/areastat/branch/master)


## 1. Introduction.

  This programm can be used to create statistics for msg, squish and
  jam message areas.

  The echostat can generate 12 types of statistics (by name, by from, by to,
  by from -> to, by size (in bytes), by quote percent, by subjects, by date,
  by week day, by time and summary statistics for any area).
  Allows to create statistics for any days, weeks or months.

  Areastat is part of the Husky Fidonet software project. At January 2004
  it's branched from Echostat version 1.06

## 2. Using areastat.

  First, You should create config file for areastat. Look th areastat.cfg
  for example configuration. If you uses POSIX-compatible operation system
  (e.g. FreeBSD, GNU/Linux) You may run following command to generate area
  list for areastat config file:

  grep -i echoarea $FIDOCONFIG | \
  awk '{print "Area " $2 " " $5 " " $3 " "  $2 ".stt";}' >> areastat.conf

  Syntax to run areastat:

  areastat cfg_name

  where
    cfg_name - confuguration file

## 3.  License.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program (look a file COPYING), or you may download
  it from http://gnu.org.

## 4. Authors and contributors

  Author or the Echostat is Dmitry Rusov
  E-Mail: rusov94@main.ru
  Fido: Dmitry Rusov, 2:5090/94

  Thanks by Dmitry Rusov to:
  Oleg Parshin, 2:5090/29
  Serge Leontiev, 2:5090/64.101

  Areastat (after Jan 8, 2004) is modified by:
  Dmitry Sergienko, Michael Chernogor, Andry Lukyanov, Stas Degteff and
  other members of [Husky](https://github.com/huskyproject) development team.
