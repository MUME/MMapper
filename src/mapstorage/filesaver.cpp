/************************************************************************
**
** Authors:   Thomas Equeter <waba@waba.be>
**
** This file is part of the MMapper2 project. 
** Maintained by Marek Krejza <krejza@gmail.com>
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file COPYING included in the packaging of
** this file.  
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
*************************************************************************/

#ifndef WIN32
# define UNIX_SAFETY 1
#endif

#ifdef UNIX_SAFETY
# include <unistd.h>
# include <errno.h>
# include <string.h>
# include <stdio.h>
#endif

#include <stdexcept>
#include "mapstorage/filesaver.h"

namespace
{
  const char *c_suffix = ".tmp";

  void throw_sys_error()
  {
#ifdef UNIX_SAFETY
    char error[1024] = "";
    strerror_r( errno, error, sizeof( error ) );
    throw std::runtime_error( error );
#endif
  }

}

FileSaver::FileSaver()
{
}

FileSaver::~FileSaver()
{
  try
  {
    close();
  }
  catch ( ... )
  {
  }
}

void FileSaver::open( QString filename )
{
  close();
  
  m_filename = filename;
  
#ifdef UNIX_SAFETY
  m_file.setFileName( filename + c_suffix );
#else
  m_file.setFileName( filename );
#endif

  if ( !m_file.open( QFile::WriteOnly ) )
    throw std::runtime_error( m_file.errorString().toStdString() );
}

void FileSaver::close()
{
  if ( !m_file.isOpen() )
    return;

  m_file.flush();

#ifdef UNIX_SAFETY
  if ( fsync( m_file.handle() ) == -1 )
    throw_sys_error();

  if ( rename( QFile::encodeName( m_filename + c_suffix ).data(),
               QFile::encodeName( m_filename ).data() ) == -1 )
    throw_sys_error();
#endif

  m_file.close();
}
