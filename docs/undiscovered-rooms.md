# Undiscovered room dots

Enable **Preferences → Graphics → Mapping Hints → Show undiscovered rooms**.
The option is off by default and can be enabled while playing.

A red corner dot marks a mapped room whose server ID is absent from the current
character's complete `Room.Known` list. MUME considers rooms seen through `exits`
known as well as rooms entered: these dots show undiscovered rooms, not a strict
record of rooms never physically entered.

Rooms without server IDs, temporary rooms, and rooms whose status is not yet
available are not marked as undiscovered. If the existing missing-ID option is
enabled, its yellow dots still identify rooms needing IDs. Where a room also has
an unsaved-change dot, the red dot takes precedence while discovery hints are on.

A per-server, per-character cache supplies the previous room history as soon as
GMCP identifies the character. It is provisional: the complete `Room.Known.List`
received in the background replaces it, including removing stale IDs. The first
load without a valid cache may take a minute or more at the server's delivery rate.
Enabling the checkbox shows an OK/Cancel confirmation explaining this initial
load, with OK selected by default. Its "Don't show again." checkbox is remembered
across restarts when confirmed with OK. Cancel leaves both settings unchanged. Opening
the preferences or loading saved settings does not show the confirmation.
If the option is already enabled at login or character switch, a missing or invalid
cache also triggers this warning once for that character session, unless warnings
have been disabled. A usable cache or an already completed server history needs no
warning. Cancel turns off discovery hints; background cache updates may continue.
The shared dialog follows MMapper's non-blocking message-box ownership pattern,
without requiring Asyncify or nested event loops. Its heading is included in the
body because macOS ignores message-box window titles. All controls are configured
before opening the dialog.

Live `Room.Known.Add` messages update the view and cache directly, even when the
overlay is disabled. Cache reads and atomic file replacements run on one serial
background worker per server cache. Server changes retain outstanding workers
until idle, so switching connections does not wait for disk I/O. Writes are
coalesced over two seconds and the final pending updates flush on normal application shutdown. An abrupt process or machine crash
can lose recent updates; the next GMCP refresh repairs the cached view.
Incomplete, unavailable, or malformed server lists never replace a good cache.
Without a cache, no red dots appear until the first list finishes.

Caches are compact, versioned JSON files below Qt's standard application-data
folder in `known-rooms`. Hashed filenames and embedded identity checks separate
characters and server host/port pairs. Invalid or incompatible files are ignored.
Late disk reads cannot overwrite another character or a completed server refresh.
Nothing is added to the map file. The code uses Qt's portable file APIs; durable
browser storage across page reloads needs an IndexedDB/IDBFS integration before
this file-based cache can be relied on in a browser. The ordinary WebAssembly
filesystem is temporary; a successful file write alone does not make it durable.

The implementation uses persistent sets for inexpensive immutable snapshots and
reuses the existing asynchronous highlight batch and dot texture. Receiving a
newly known room does not invalidate terrain meshes. Completed batches are keyed
by map, display options, and discovery revision so a batch for another character
cannot be shown. Native and WebAssembly builds use the existing rendering path;
there are no additional dependencies or artwork files. The supported browser build
uses Qt's multithreaded WebAssembly kit (see `Dockerfile`); this patch does not
introduce a separate threading requirement or claim single-threaded browser support.

Highlight invalidation retains an in-flight worker instead of destroying its
`std::async` future, whose destructor can block. There is at most one highlight
worker at a time. Changes are coalesced, and results for incompatible display
states or characters are rejected before rendering. Waiting during application
destruction is still permitted so workers cannot outlive their owners. The renderer accesses OpenGL only on the GUI thread;
workers consume immutable snapshots and produce vertex data.

Portability: the code uses the project's existing C++20, Qt, and Immer facilities,
fixed-width server IDs, and the shared desktop/WebGL rendering path. No operating
system APIs, new shader features, map-format changes, or new assets are required.
The expanded named-color capacity is a 1 KiB uniform block. Desktop caches use
Qt file APIs; browser persistence has the limitation described above.

Protocol reference: <https://mume.org/help/gmcp_room.known>.

## Validation

`TestProxy` covers chunked histories, live additions during and after transfer,
immutable snapshots, character changes during transfer, disabled/unavailable
states, empty histories, malformed IDs and lists, invalidations, and observer
notifications. Cache tests also cover restart, stale-ID removal by server truth,
live additions during refresh, final-write flushing, interrupted refreshes,
character/server separation, case-insensitive cache keys, missing-cache warnings,
and invalid files. Standard GMCP serialization/module tests remain in the same suite.
`TestMainWindow` checks the shared dialog's default button, cancellation, remembered
warning preference, and automatic deletion.

Manual game checks:

1. Enable the option after logging in and wait for the history to finish loading.
2. Enter or discover a marked room and verify its red dot disappears.
3. Switch characters and verify that the old character's dots are not displayed.
4. Toggle the option alongside the existing yellow/blue mapping hints.
5. Disconnect or use a room without a server ID and verify no discovery claim is made.
