

/************************************************************************************
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
************************************************************************************/

#pragma once

//define EPORT_SSL_ENABLE
//define EPORT_ZLIB_ENABLE
//define EPORT_KCP_ENABLE

/***********************************************************************************/
// eport for c++ library
// eport::io_context
// eport::ip::udp::socket
// eport::ip::tcp::socket
// eport::ip::tcp::stream (tcp/ssl/ws/wss supported)
// eport::ip::tcp::acceptor
// eport::ssl::context
/***********************************************************************************/

#include "eport/version.hpp"
#include "eport/detail/factory.hpp"
#include "eport/detail/io/endian.hpp"
#include "eport/detail/io/context.hpp"
#include "eport/detail/io/buffer.hpp"
#include "eport/detail/io/conv.hpp"
#include "eport/detail/io/parser.hpp"
#include "eport/detail/io/protocols.hpp"
#include "eport/detail/io/decoder.hpp"
#include "eport/detail/io/thread_pool.hpp"
//#include "eport/detail/coroutine.hpp"
#include "eport/detail/error.hpp"
#include "eport/detail/identifier.hpp"
#include "eport/detail/zlib/gzip.hpp"
#include "eport/detail/algo/base64.hpp"
#include "eport/detail/algo/sha1.hpp"
#include "eport/detail/ssl/context.hpp"
#include "eport/detail/timer/steady_timer.hpp"
#include "eport/detail/timer/system_timer.hpp"
#include "eport/detail/timer/high_resolution_timer.hpp"
#include "eport/detail/socket/address.hpp"
#include "eport/detail/socket/tcp/endpoint.hpp"
#include "eport/detail/socket/tcp/socket.hpp"
#include "eport/detail/socket/tcp/acceptor.hpp"
#include "eport/detail/socket/tcp/stream.hpp"
#include "eport/detail/socket/udp/socket.hpp"
#include "eport/detail/socket/udp/endpoint.hpp"
#include "eport/detail/socket/kcp/socket.hpp"
#include "eport/detail/os/os.hpp"
#include "eport/detail/httpfile.hpp"

/***********************************************************************************/
