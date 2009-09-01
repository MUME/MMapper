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

#ifndef INCLUDED_FILESAVER_H
#define INCLUDED_FILESAVER_H

#include <QFile>

/*! \brief Save to a file in an atomic way.
 *
 * Currently this does not work on Windows (where a simple file overwriting is
 * then performed).
 */
class FileSaver
{
  QString m_filename;
  QFile m_file; // disables copying

public:
  FileSaver();
  ~FileSaver();

  QFile &file() { return m_file; }

  /*! \exception std::runtime_error if the file can't be opened or a currently
   * open file can't be closed.
   */
  void open( QString filename );

  /*! \exception std::runtime_error if the file can't be safely closed.
   */
  void close();
};

#endif /* INCLUDED_FILESAVER_H */
