/*
 * This file is part of the efc project <https://github.com/eurus-project/efc/>.
 * Copyright (c) 2024 The efc developers.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SITL_TCP_CONN_H_
#define _SITL_TCP_CONN_H_

namespace efc
{
namespace sitl
{
    class TcpConnection
    {
    public:
        TcpConnection() { std::cout << "TcpConnection created.\n"; }
        virtual ~TcpConnection() { { std::cout << "TcpConnection destroyed.\n"; } }

        static TcpConnection* CreateTcpConnection();

    private:

    };
}
}

#endif