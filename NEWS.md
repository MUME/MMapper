## MMapper 25.06.2 (June 23, 2025)

### New Features:
* **Performance Improvements:**
    * **Blazing Fast Rendering**: Intermediate rendering now occurs on a background thread, slashing responsiveness time during map updates by over 10x.
    * **Lag-Free Groups**:  The Group Panel's performance was significantly improved by only updating individual rows, leading to a much smoother experience with large groups.
    * **Parallelized Updates**: The map consistency checker and `_map diff` command now run in parallel to complete 4x faster.
    * **Faster Map Changes**: Updated the underlying data structures to copy-on-write, boosting overall application performance by around 100 milliseconds.
* **Expanded Search:** The `_search` command now supports:
    * `-area` parameters to select an entire area that has been mapped.
    * `-regex` parameters for more powerful queries.
* **Revert Button:** A button has been added to allow users to revert room changes back to the previous save.

### Bug Fixes:
* **Pathfinding and Mapping Logic:**
    * In-game falls will now correctly trigger a "move" event.
    * Fixed the pathfinding machine to correctly handle connections where exits are not visible.
    * Corrected an issue causing the pathfinding machine to discard temporary rooms.
    * Support was added for more types of failed movement actions around doors and climbs.
* **User Interface and Visuals:**
    * Fixed "compact mode".
    * Corrected an issue where changes to exits would not trigger a visual remeshing of the map.
    * Fixed the restoring of window positions and sizes and ensured the main window is visible on load.
* **Functionality Fixes:**
    * Fixed the `_search` command to correctly handle special characters.
    * A failure to allocate a graphics shader is now handled correctly and will terminate the app to prevent undefined behavior.
    * The application now probes for the highest available OpenGL version on startup.
* **Crash Fixes:**
    * Fixed a crash that occurred when loading an unsupported or empty file.
    * Fixed a crash related to the "force to room" functionality.
* **Data Integrity:**
    * Implemented a fix to prevent data corruption by restoring the last map snapshot if a consistency check fails.
    * Fixed an issue where an area was not removed when its last room was deleted.
* **Build and Installation:**
    * Corrected a typo that affected the installation of the Visual C++ Redistributable.
    * Fixed the AppImage build to properly include Wayland, improving compatibility for Linux users.

### Changes:
* **User Experience Tweaks:**
    * The application's minimum window size has been reduced.
    * The splash screen is now hidden immediately upon any map modification.
* **Internal/Developer:**
    * Numerous internal code cleanups, refactoring, and style changes were made to improve maintainability.
    * Improvements were made to developer-facing tools, including the output of `_map stats` and `diff`.

---
## MMapper 25.06.1 (June 13, 2025)

### New Features:
- Added group panel enhancements and preferences page to:
  - Character re-ordering via drag and drop support
  - Hide NPCs option
  - Always sort NPCs to the bottom option
  - Override the color of all NPCs option

### Bug Fixes:
- Mapping will no longer overwrite room contents unless it is a forced update or new room
- Fixed ``_search`` to correctly handle multi-word searches
- Allowed the window size to become smaller once again
- Fixed Windows installation error by always installing the Visual C++ Redistributable
- Fixed stale characters being shown on the group panel after they were removed

### Changes:
- Migrated AppImage build system from `linuxdeployqt` to `linuxdeploy`

---
## MMapper 25.06.0 (June 11, 2025)

### New Features:

* **Modernized Parser and Group Manager (GMCP):** MMapper now uses the modern GMCP protocol for core game data and group management. This is a result of major internal refactoring of the parser and group manager to a GMCP-based system. This replaces older peer-to-peer methods, improving overall reliability and accessibility. For groups, this means **no more hassle with sharing IP addresses or configuring firewalls!**
* **Full Unicode and Emoji Support:**
   * **Modern Character Encoding:** MMapper now fully supports Unicode internally, thanks to a significant backend overhaul and refactoring of internal utilities. This solves problems with incorrectly displayed characters in names, descriptions, or notes, and brings full Emoji Support.
   * **Emoji Shortcodes:** Express yourself better! You can now use familiar icons and emoji shortcodes like `[:+1:]`, `[:smiley:]`, or `[:skull:]` in your notes or other text areas, and even in text sent to MUME to mark danger zones or jazz up tells. MUME will receive emojis as a shortcode and MMapper will translate it back to emoji on UTF-8 supported clients.
* **Secure Login Credential Saving:** Tired of typing your password every time? You can now securely save your MUME login credentials using your operating system's keychain, making logging in quicker and avoiding the need to store passwords insecurely.
* **Improved Connectivity & Distribution:**
    * **WebSocket Connection Option:** Struggling to connect through a strict firewall? MMapper can now connect to MUME using WebSockets as a fallback method, improving connectivity for users behind certain network restrictions. This is supported by a rewritten proxy pipeline that handles client-server connections more robustly.
    * **Flatpak Distribution:** MMapper is now available as a Flatpak, simplifying installation and updates across various Linux distributions.
    * **Apple Silicon Support:** MMapper now offers a native package for Apple Silicon Macs, providing optimized performance and compatibility for the latest Apple hardware.
* **Mapping Enhancements:**
    * **Missing Map ID and Unsaved Changes Dots:** MMapper now supports MUME's per-room Map IDs that once discovered allow for deterministic room identification within Group Manager on different maps. MMapper will display yellow dots where a Map ID is missing and blue dots where there are unsaved changes to help you populate your map with Map IDs.
    * **Automatic Door Mapping:** Mapping hidden doors has been made easier due to how MMapper now automatically does this for you. If you want to not have MMapper automap a door name mark the exit as No Match.
    * **Additional Map Data Tracking:** MMapper now tracks areas, artificial light sources, and nice weather conditions reported by the game.
    * **Manage Unknown Exits:** You can now remove exits that are marked as "unknown" from your map data.
* **Client & UI Additions:**
    * **Live Scrollback Preview:** While reviewing scrollback, you can now still see the latest game output — making it easier to keep track of what’s happening in real-time. The built-in client also supports clickable URLs and ITU underline styles sent by the game.
    * **Improved Input in the Integrated Client:** The optional built-in MUME client has received usability improvements, including function keys, smarter `TAB` autocomplete, better `CTRL+TAB` behavior, and a clearer visual distinction for password input fields.
    * **Description Panel:** New panel shows descriptive text and supports GenAI artwork via [Modding](https://github.com/MUME/MMapper/wiki/Modding). Many thanks to Freya who created the default images!
      * Room art: ``rooms/<mapid>.jpg``
      * Area art: ``areas/<normalized-area>.png``
        * Normalization: lowercase, remove the first ``the ``, convert Latin-1 to ASCII, replace spaces with ``-``
        * Example: ``the Lhûn Valley`` → ``areas/lhun-valley.bmp``
        * Images can be either PNG, JPG, or BMP formats.

### Changes & Improvements

* Improved ``_map stats`` output with more color for easier reading and quick analysis.
* Switched to zlib-ng on Windows: 2× faster compression.
* Map loading and saving are now asynchronous, preventing UI freezes, a direct benefit of the backend overhaul for asynchronous operations.
* Remote editing now uses MUME.Client GMCP instead of the legacy MPI protocol, leading to a more robust parser.
* Connection attempts are faster due to a reduced timeout.
* Editing room notes feels smoother with less frequent updates during typing.
* Group state icons are cached for quicker display.
* The "Edit Room Attributes" dialog no longer blocks interaction with the rest of the MMapper window, allowing you to multitask more effectively.
* Replaced the path machine state from logs with a status bar widget, offering a more streamlined view.
* Removed beta branding from the Adventure Panel.
* Removed the old splash screen and integrated it directly into the map window.
* Made the log panel read-only to prevent accidental font resizing.
* For advanced users, the `MMAPPER_WINDOW_TITLE_PROGRAM_FIRST` environment variable allows you to change the order of elements in the window title, providing flexibility for specific window management setups.
* Deathtrap terrain type rooms have switched from a terrain type to a load type. Older map files will now have these deathtraps loaded as the indoors terrain type to better match MUME's output.
* Corrected problems with how certain special characters and text encodings (like UTF-8 quotes and XML entities) were handled and displayed, thanks to the Unicode support improvements. As a result, XML passthrough has been removed.
* ``_open`` and ``_close`` door commands no longer use door names to encourage the use of in-game door memory and ensure fair play.
* Updater enhanced to check beta or release channels.

### Performance & Stability

* General performance improvements across the application, partly due to threading model changes that eliminated dedicated threads for the Group Manager and optional proxy, leading to a more efficient concurrency model.
* Addressed several potential crashes and stability issues related to data handling, timers, and parser exceptions, benefiting from widespread C++ modernization and stricter thread safety enforcement.

### Under the Hood

* **C++ Modernization:** There was a widespread effort to modernize the C++ codebase, adopting practices like RAII (Resource Acquisition Is Initialization) with custom handlers, using `noexcept`, preferring iterators over raw pointers, avoiding `new`/`delete`, using modern C++ features, and applying patterns like the "badge" idiom.
* **Build System Enhancements:** The build environment was updated to Ubuntu Jammy for AppImages and enabled the Mold linker to improve build times.
* **Dependency Streamlining:** The MiniUPnPc dependency was removed, simplifying the build and reducing external requirements.
* **Windows Support:** Release packages are now built using Visual Studio and the Windows installer has been improved to prompt for closing MMapper during the uninstallation phase.

---
## MMapper 24.03.1 (March 11, 2024)

### Bug fix:
 - Do not send MSSP to clients that haven't requested it. This fix helps JMC users.

## MMapper 24.03.0 (March 10, 2024)

### New features:
 - MMapper will now synchronize the time to the hour using MUME's Mud Server Status Protocol. (Mirnir)
 - The recent whitespace change is now compatible with MMapper where room descriptions are now normalized.

### Bug fixes:
 - Timers will now trigger for Elves and Half-elves. (Gamor)
 - Some memory leaks were fixed.

### Changes:
 - The minimum supported version of macOS is now Monterey.
 - The Web Map format has changed its md5 hashing strategy to also normalize on whitespace.
 - The unit test coverage was improved by 3%. (Gamor)

---
## MMapper 23.05.0 (May 1, 2023)

### New features:
 - Added support for a rattlesnake mob flag to differentiate between the
   attention flag
 - The menu bar can now be intelligently hidden until mouseover for a more
   minimal experience
 - Rattlesnake, attention, and smob hints are now displayed in the
   emulated exits

### Bug fixes:
 - The adventurer panel has a corrected XP and TP hourly rate
 - Undead kills are now tracked in the adventure panel
 - The moon has been fixed to follow the game's adjusted synodic month

### Changes:
 - Time is now synchronized using GMCP Event.Darkness and Event.Sun
 - Moon phases are now considered as part of the visibility counter

## MMapper 23.03.0 (March 18, 2023)

### New features:
 - Added the Adventure Panel that helps players track, organize, and
   understand their adventures such as hints, XP, and achievements (Taryn)
 - Added the `_timers` command for players to track things (Azazello)

### Bug fixes:
 - Fixed crash when editing multiple rooms
 - Fixed Warrens' dawn and dusk messages (Troth)
 - RoomPanel now supports dark mode on macOS and Linux
 - Use Char.Vitals GMCP to set riding or position state rather than from the
   prompt using regular expressions

### Changes:
 - Internal commands don't have weird output like "--->" anymore
 - Github Actions is now used very extensively for continuous build by the
   MMapper developers
 - Windows build has stopped using MSYS2 because it upgraded to OpenSSL3 which
   is not supported by the Qt5 framework

## MMapper 23.01.0 (January 1, 2023)

### Bug fixes:
 - Fixed a prompt regression where it felt slower

---
## MMapper 22.12.1 (December 31, 2022)

### New features:
 - Scroll bars and the status bar can now be hidden under the View menu. (Taryn)

### Bug fixes:
 - Fixed crash in `_dirs` command (Elval)
 - Prompt is now drawn correctly by supporting nested XML tags

### Changes:
 - Mapped rooms don't have dangling newlines anymore
 - MMapper now requires MacOS 11 Big Sur
 - .deb Linux package is now built on Debian Bullseye

## MMapper 22.12.0 (December 4, 2022)

### Changes:
 - Default map has been updated to support the Bree-land and parts of ABR

### Bug fixes:
 - Improved stability of remote editor
 - Base map filter supports Mandos
 - Removed Top Mud Sites from the `_vote` command and menu
 - Fixed shader error on AMD video cards using driver 22.11.1

## MMapper 22.05.0 (May 7, 2022)

### New features:
 - Characters in the room can now be viewed in the Room Panel. This panel
   populated by the GMCP Room.Chars module (Cosmos)
 - Maps can now be exported and imported using the new human-readable and
   editable mm2xml XML format (Cosmos)

### Changes:
 - Default map has been updated

---
## MMapper 21.12.1 (December 6, 2021)

### Changes:
 - Mapping only trusts Orc-mode to set room SUNDEATH
 - Mapping only trusts Troll-mode to set room NO_SUNDEATH, LIT, DARK
 - Removed OpenGL ES support because the Raspberry Pi 4 supports OpenGL 2.1

### Bug fixes:
 - Room syncing works now for Orcs when exits are sunny
 - Short prompts without the light and terrain are now correctly parsed
 - Fixed UTF-8 to Latin-1 encoding bug where special characters showed up as ??
 - Mac build works again by switching to brew miniupnc and qt5
 - Added work-around for TinTin++ character encoding negotiation

## MMapper 21.12.0 (December 2, 2021)

### New features:
 - MMapper will try to reduce sundeaths by resetting the clock to lowest
   precision when re-connecting. Users can override this by clicking the clock
   manually at their own risk.
 - Live remote edits will now prompt to have their contents saved if the
   connection to MUME is disrupted rather than disappearing into the Void.

### Changes:
 - Mac builds now require Mac OS X 10.15 Catalina at a minimum.
 - Deb packages are now Ubuntu 20.04 (amd64) or Debian Bullseye (arm64).
 - Minimum supported Qt and CMake are now 5.12.4 and 3.16 respectively.
 - Cotire has been replaced with Unity Builds decreasing build times by 30%.

### Bug fixes:
 - Powtty now provides the Terminal Type to MMapper as "XTERM".
 - Eliminated newlines that were added for each XML tag (i.e. <character> or
   <magic>) in the saved room contents.

## MMapper 21.09.2 (September 29, 2021)

### Bug fixes:
 - Prevent reconnecting from kicking players in Group Manager
 - Fix Latin-1 character mojibake in Group Manager names
 - Prevent scouting from occasionally triggering movement

## MMapper 21.09.1 (September 26, 2021)

### New features:
 - Use Char.Name GMCP module to populate the Group Manager
 - Use Char.StatusVars GMCP module to detect sundeath exits

### Bug fixes:
 - Prevent renaming from kicking players in the Group Manager
 - Update prompt parsing to support XP and TP TNL
 - Retain GMCP modules between reconnects
 - Fix race condition when proxying External.Discord.Hello

## MMapper 21.09.0 (September 24, 2021)

### New features:
 - Balrog's whip entangle "bash" is now detected in the Group Manager
 - "score report" output is now captured for the Group Manager
 - Add the ability to search using a regex in the Find Rooms dialog
 - [Beta] Use Char.Vitals GMCP messages to populate the Group Manager when
   MUME adds support for GMCP Char module

### Changes:
 - Rename MMapper.GroupTell to MMapper.Comm.GroupTell
 - Stopped sending the IAC-GA prompt request MPI to MUME

### Bug fixes:
 - Prompt rewriting works
 - Light and terrain characters in the prompt are now optional since we now
   parse the terrain type from <room terrain=...> XML attribute
 - Fixed RFC 2066 Charset negotiation
 - Refactor telnet option handling to be less spammy

## MMapper 21.08.0 (August 9, 2021)

### New features:
 - Added a new milkable mob flag and updated the default map to support it
 - Room textures can now be modded by players: [https://github.com/MUME/MMapper/wiki/Modding](https://github.com/MUME/MMapper/wiki/Modding)
 - Added the `_mark` command to add, update, or remove text marks without using a mouse
 - Distant selected rooms are now scaled based upon distance

### Changes:
 - Renamed `_markcurrent` to `_room select`
 - Anonymously exposed OpenGL version in the terminal type

### Bug fixes:
 - Fixed MCCP2 compression crash when reconnecting
 - Prevent colors from looking washed out on Mesa 21.0.3 by not disabling multisampling

## MMapper 21.06.0 (June 1, 2021)

### Note:
 - Windows users should manually uninstall MMapper from "Add or remove
   programs" before installing the new MMapper.
 - MMapper has dropped support for 32-bit binaries.

### Changes:
 - Default map now includes Dol Guldur environments
 - Fixed <header> XML support parsing
 - Improved Windows installer High DPI support
 - Supported macOS 10.11 networking better
 - Increased telnet socket timeout to 30 seconds
 - Fixed an ANSI color encoding typo on the welcome message
 - Renamed `_removedoornames` to `_remove-secret-door-names`
 - Fixed RoomEditDialog multiselection rendering error
 - Squished various minor bugs

## MMapper 21.01.0 (January 17, 2021)

### New features:
 - Play, emulation, and mapping modes can be changed with: `_config ??`
 - An insecure connection warning will now be displayed if TLS encryption is disabled
 - Selecting rooms and marks now displays the total count

### Bug fixes:
 - Fix autoexits parsing to support the new MUME XML format
 - Room online update status can now be changed when multiple rooms are selected
 - UI now allow remote and local ports above 10000
 - Connect room(s) to its neighbour rooms now works with up/down
 - Allow clock to sync with Ainur
 - Fixed rare crash when a connection was not bi-directional
 - The world mesh is now updated during a tolerant room updates
 - Only troll exits are trusted for detecting direct sunlit exits, thereby improving accuracy
 - Removed the confusing orange bounds that resulted in a mesh rebuild
 - The display is now updated when changing a room's up to date flag.
 - DPI is checked when MMapper is started up
 - Fix _dirs command by swapping n/s

---
## MMapper 20.10.0 (October 13, 2020)

### New features:
 - Exits are now automatically generated during "Create New Connection" modes
 - Connections are now automatically deleted when a room exit is removed
 - Introduced End of Record (EOR) telnet protocol for Mudlet
 - Added Linemode (RFC 1116) telnet protocol for Putty and BSD Telnet
 - Selecting rooms and marks now displays the total count

### Bug fixes:
 - Fixed 'Connect room(s) to its neighbour rooms' to connect north/south and one-way exits
 - Selecting the "Undefined" radio buttons when editing multiple rooms now works
 - Connection dots when selecting outgoing exits are now correctly displayed
 - Fixed directory creation when exporting a web map for 'Play MUME'
 - Added Top Mud Sites link to `_vote` command
 - Suppress Go Ahead (SGA) telnet protocol now actually works
 - Updated links to use HTTPS

## MMapper 20.08.0 (August 9, 2020)

### New features:
 - MMapper can now automatically log play sessions to disk
 - Reworked underwater terrain sectors to be more identifiable
 - Flow flags now predict the movement into the next river room
 - Group Manager pulls health, mana, and move information from 'info' as well
 - AppImage Linux binaries are now packaged

### Bug fixes:
 - Increased draw distance for maps with many layers
 - Fixed setting door names using the CLI
 - Reduced the default client font size on Linux/Windows
 - Fixed bug where twiddlers were not being prompt detected

## MMapper 20.05.0 (May 20, 2020)

### New features:
 - Support for Generic Mud Communication Protocol (GMCP) and MMapper.GroupTell module
 - Support for Mud Client Compression Protocol (MCCP2)
 - Ability to change the color of normal connections

### Bug fixes:
 - Fixed MUME clock and time output parsing
 - Remote edit widget justifies at 80 characters rather than 81
 - Group poison affect now triggers on any poison message in status
 - Prompt and exits detection has been improved
 - External links use HTTPS over HTTP
 - [Mud Connector](https://www.mudconnector.com/) voting works again
 - Disabled unnecessary scrollbars on MMapper logo in About dialog
 - Linux snap now runs successfully on the Raspberry Pi

## MMapper 20.03.0 (March 28, 2020)

### New features:
 - Default map now includes Southern Mirkwood zones
 - Overhauled textures to be 250% larger and have more detail
 - Introduced 150% fractional scaling support for users with 4K monitors
 - Added a browse button to select 3rd party programs as your remote editor
 - Reduced Mac security dialogs by only listening on the local interface by default

### Bug fixes:
 - Screen DPI changes are now detected when MMapper is moved across monitors
 - Preferences dialog will now reposition itself on each open
 - Fixed off-by-one error in the internal remote editor
 - Fixed the saving of new maps to be less tedious with each subsequent save
 - Fixed a loop where players could get stuck as incapacitated
 - PowTTY terminal type will now include more information than simply 'unknown'
 - Squished bugs around snooping for Valar

---
## MMapper 19.12.1 (December 31, 2019)

### Bug fixes:
 - Fix missing messages for hunger/thirst on group manager
 - Group manager affects are now always visible for new group members
 - Update dialog will now correctly close and download the relevant upgrade link

## MMapper 19.12.0 (December 28, 2019)

### New features:
 - Added search, riding, and snared affects to the group manager
 - Mouse clicks on a room during 'mouse mode' now display an information tooltip
 - 'Find Rooms' dialog will now remember its last window position and dimensions
 - Room mapping commands now use the syntax model. For more info type: _room ??
 - Group manager commands now use the syntax model. For more info type: `_group ??`
 - Integrated mud client is now a panel and not a non-modal window
 - Integrated mud client now uses a centered tooltip for dimension hints
 - Trilinear filtering is now enabled by default
 - Promote the _connect command when MUME disconnects

### Bug fixes:
 - Fixed ancient bug where mudlle'd movement would break map syncing (GH gate, Lorien, etc)
 - Always display exits even if the player is in an unknown room
 - Changing the character encoding within the preferences does not require a restart anymore
 - The last remembered prompt is now correctly reset on disconnect
 - External remote editor support is now working
 - Scrolling on the map using the scrollbar buttons now move the map by a single room
 - Special commands now take effect before generic door commands
 - Refactored group manager and parser code to be more correct and maintainable
 - Group manager now correctly extracts the external IP
 - Fixed integrated mud client preferences page to restore all settings
 - Fixed restoring main window size on startup
 - Fixed regression with internal mud client up/down key press behavior on Mac
 - Fixed room syncing for Valar

## MMapper 19.10.1 (October 31, 2019)

### Bug fixes:
 - Remapped 'Road to the Grey Havens' on default map to resolve syncing
 - Preferences dialog is larger again
 - Fixed high DPI scaling on Windows

## MMapper 19.10.0 (October 25, 2019)

### New features:
 - Faster rendering for most zoom levels with new textures and fonts
 - "3D view" with optional tilting as you zoom in
 - Enhanced marker and room connection editing
 - Improved map indicators for the character and group members
 - Added visible map mode bounds to decrease draw latency
 - Rendering now supports OpenGL ES (e.g. Raspberry Pi 4)
 - Added new `_connect` and `_disconnect` commands
 - Removed old room commands in favor of new room commands
 - Saving maps is now disabled unless the map was changed
 - Clicking on a group character causes map to center on them
 - Fall rooms are now supported during map and play mode
 - Exit parsing now works for Maiar to aid in zone building
 - Improved security by supporting OpenSSL 1.1.1
 - Clock now displays a warning emoji if it has not been synced
 - Releases now generate checksum files for packages
 - View menu has a new 'Reset Layer' entry
 - About dialog displays all licenses for included resources and libraries
 - Clock now predicts and displays the phase of the moon
 - Search command and Find Rooms dialog can now search flags
 - Character state and affects are now shown in the group manager
 - Group tells can use the character's ANSI 256 color if your client supports it
 - Introduced support to import Pandora maps and export MMP maps
 - Unselecting rooms can now be done via commands (i.e. _search -c)

### Bug fixes:
 - Fixed left-click movement/panning precision error
 - Graphics avoid wrapping grass onto roads at room edges
 - Settings dialog is no longer modal
 - Always display "no match" exit flags
 - Fixed room dragging speed to be less aggressive
 - Mouse wheel zoom is now centered on the mouse cursor
 - Fixed regression during mapping that left rooms locked
 - It is now impossible to kick yourself in the Group Manager
 - Group Manager clients assume the first received character is the host
 - Special commands are now allowed to have mixed case (i.e. `_HeLp`)
 - Factory reset will now refresh the preference dialog settings
 - Fixed MUME vote link to The Mud Connector
 - Fixed regression that made the parser less tolerant of secret exits
 - All map changes now actually ask if you want to save on close
 - Fixed 'persistent room' bug during mapping that would leave extra rooms on the map
 - Connection action 'Connect to neighbors' will only connect rooms if there is a valid exit
 - Default configuration setting for 'Software rendering' is now 'off' on Windows
 - Fixed bug that would migrated old settings from being migrated during a factory reset
 - Infomarks and rooms can now be simultaneously selected via right click
 - Emulated exits now use movement hint to select the correct room for followers
 - Correctly raise, focus, and activate remote edit windows to the top of the window manager if the OS allows it
 - Enforce that character names should start capitalized
 - Tweak group manager character color brightness threshold to favor black text
 - Fixed Latin-1 character encoding bug in group tells
 - Improved integrated client autocomplete functionality to also clear
 - Lower clock precision if there has not been a sync within the last real life day to prevent clock skew
 - Fixed update checker to correctly compare version components
 - The mouse wheel now behaves consistently across mouse modes
 - Cleaned up connect/disconnect text

## MMapper 19.04.0 (April 22, 2019)

### New features:
 - Updated map to include the Tower Hills
 - Improvements to the builtin editor (justification, whitespace, ansi, tabbing, and more!)
 - Group manager now shows player prespam
 - Add `_knock` action to parser
 - Colors for dark/sunsafe rooms are now configurable under the Graphics preferences
 - Introduce coach and ferry load flags
 - Group manager rows and columns are smaller and autohide if necessary
 - Group manager remembers previous hosts as Authorized Contacts
 - Improved room texture resolution by 2x through artificial intelligence

### Bug fixes:
 - Really fix syncing when player is blinded
 - Remove group manager UPnP mapping from the router on shutdown
 - Mapper now syncs movement when a one-way is scouted
 - Connected room flags are now reset for followers with mapping mode
 - Add more common failed movement message to prespam
 - Update group position when movement is forced (i.e. river flow)
 - Remote edit widget now grabs focus
 - Only show rooms as distant if they are not being moved
 - Fix rendering artifacts when swapping rooms in room editor
 - Reset group manager character info on MUME disconnect
 - Changing the client font is now correctly saved

## MMapper 2.8.0 (March 7, 2019)

### New features:
 - Remote edit justify now understands ANSI and various lengths
 - `_search` and `_mark` commands now select and show distant rooms
 - Map immediately reflects changes from internal commands (i.e. `_noride`)
 - MMapper now checks for upgrades on Github

### Bug fixes:
 - Fix syncing when player is blinded
 - `_dirs` command is now aware of damage/fall exits and tries to avoid them
 - Prespammed directions are more resilient to non-movement commands
 - Internal commands now trigger on the tail position of prespam (i.e. `_open`)
 - Blacklist certain OpenGL drivers and fallback to software rendering on Windows
 - Fix "Always on top" action
 - Package missing msvcr120.dll on Windows

## MMapper 2.7.4 (January 2, 2019)

### New features:
 - Group manager hosts will now have their ports automatically forwarded using UPnP
 - A red highlight is displayed below remote edit text that needs to be justified
 - Remote edit widget can now justify text to 80 characters (like MUME's %j)
 - Added toggle to prevent the group manager from autostarting

### Bug fixes:
 - Improved Mac OS X 10.9 and dark mode support
 - Refactored configuration to support multiple profiles for power users
 - Log view now correctly scrolls down on updates
 - The account "time" command will not unsync the clock
 - Last remembered prompt is now cleared on disconnect

---
## MMapper 2.7.3 (December 26, 2018)

### New features:
 - Allow TLS connections to be compressed
 - Crash reporting added for Windows
 - Added `_glock` command to toggle the group lock

### Bug fixes:
 - Improved group manager stability after the host disconnected
 - Group manager host disconnecting will now not cause a message box to appear 3x
 - Major refactor of offline character movement and rendering pipeline

## MMapper 2.7.2 (December 8, 2018)

### New features:
 - Disabled NAGLE for tcp connections which should hopefully increase performance during lossy conditions
 - ANSI color selection has been moved into a dialog that supports high colors
 - Internal command prefix character can be changed with "`_set prefix <char>`"
 - Group manager clients will attempt to reconnect 3 times to a host before failing
 - Group hosts can lock the group to the current clients
 - Group tells are now colored

### Bug fixes:
 - Fixed crash due to threading issues on Windows
 - Tightened TCP keepalive to hopefully prevent idle connections from dropping
 - Fixed "black screen bug" with Intel video cards on Windows
 - Group manager reconnects are more secure and verify the entire OpenSSL certificate matches
 - Group manager prevents clients from spoofing another client
 - Parser detects and remembers twiddlers, logins, and account prompts
 - Preference sections are visually disabled when unselected
 - Improved High DPI display support
 - Discrete nVidia and AMD GPUs are preferred on Windows for laptops with hybrid graphics
 - Room and group selection has been refactored

## MMapper 2.7.1 (November 30, 2018)

### New features:
 - Info Markers are now selectable and movable using a mouse
 - Disconnects from MUME are now optionally mirrored on the client
 - Pinch gestures now zoom the map
 - Store and validate additional Group Manager secrets metadata

### Bug fixes:
 - Group Manager now remembers its last window position and state on boot
 - Only hidden door names are displayed on the map now
 - View panel moved to Sidebars toolbar
 - Door commands now work without a direction if there is only one secret exit
 - Prompts now update HP, mana, and moves using a lower bound

## MMapper 2.7.0 (November 24, 2018)

### New features:
 - Group manager keeps your communications secure with encryption
 - Group manager allows you to authorize who can connect to your group
 - Elite and super mob flags have been introduced
 - Quest and passive mob flags have been modernized
 - Word of recall and equipment load flags have been added
 - 'Find Rooms' dialog can now select and edit rooms
 - Hosts can kick group members with the `_gkick` command or by clicking on them
 - Offline mode now supports scouting and random exits
 - Integrated editor shows the line/column of the cursor within the status bar
 - Rooms can be forced updated or outdated
 - 'Edit Rooms' dialog icon and flag ordering improvements

### Bug fixes:
 - Improved stability when editing rooms after force moving
 - Multiple room and load flags are displayed at the same time again
 - Fixed bug where group member's mana was always shown as zero
 - Squished bug with prompt handling when the connection was not encrypted
 - Removed deprecated 'random' terrain type that displayed warnings in the logs

## MMapper 2.6.3 (November 12, 2018)

### Changes:
 - *Orc* day/night strings now sync the Mume clock
 - Fix Parser configuration ANSI dropdown to correctly select "none"
 - Introduced button to perform a configuration factory reset
 - Default integrated client font is a bundled DejaVu Sans Mono font
 - Fixed a race condition in the group manager and refactored the code
 - Do not draw characters if they are in an unknown location

## MMapper 2.6.2 (October 27, 2018)

### Changes:
 - Fixed bug when the negotiated window size was larger than 127 characters
 - Prompts are more consistently stored and displayed with internal commands
 - `_note` command now clears notes when provided with an empty payload
 - Added shortcut to reset the zoom level back to the default level
 - Darkened default background color

## MMapper 2.6.1 (October 12, 2018)

### Changes:
 - Fixed crash when user selected US-ASCII character encoding
 - Improved stability when player was in room that got deleted
 - Certain InfoMark classes now have a colored background rather than text
 - Forcing the path machine to update rooms works again
 - Improved Telnet parsing to better support negotiated window size
 - Re-ordered window tab navigation to make sense
 - MacOS Mojave and Wayland support improved

## MMapper 2.6.0 (September 29, 2018)

### Note:
 - MUME will now ignore 'change charset' commands because MMapper 2.6.0 always
   communicates to MUME in Latin-1 over a Telnet proxy
 - In order to change the character encoding sent to your mud client please
   navigate to the MMapper General preferences and select an alternative
   character encoding from the drop down: Latin-1, UTF-8, or US-ASCII

### Changes:
 - Introduced "No bash" door flag and "Stables" room flag
 - Squished bug where rooms would update themselves outside of "Mapping Mode"
 - Integrated client now displays user input as yellow
 - MMapper now supports character set negotiation (RFC 2066) and handles
   character encoding for any client that supports it
 - Prompts are now parsed if they are coloured
 - Windows users now utilize OpenGL software rendering by default
 - Added "Show launch panel" option to General preferences
 - Stripped support for maps predating MMapper2
 - Fixed Parser & bug in 'Suppress XML' mode
 - TLS certificate information is now logged
 - Refactored room lookups to use STL data structures
 - Cleaned up MMapper configuration and map compression logic

## MMapper 2.5.3 (August 18, 2018)

### Changes:
 - Group Manager stability improved
 - Proxy threading re-enabled by default and exposed as an option
 - Improved error reporting when connecting to MUME
 - Corrected XML entities when unchecking the 'Suppress XML' option
 - Emulated exits better support Latin-1 and UTF-8 character sets
 - Up/down one way exits are rendered again
 - Greatly improved path machine performance back to 2.5.0 levels
 - Path machine handles doors/roads/climbs better

## MMapper 2.5.2 (August 12, 2018)

### Changes:
 - Host disconnecting will no longer cause Group Manager to crash
 - Prespammed path now correctly displays on the map
 - Emulated exits are better sync'd due to threading changes in the Proxy
 - Exits do not mangle special characters for the UTF-8 character set
 - Edit room dialog shows and selects terrain correctly
 - Dialogs should now consistently be visible on the viewport
 - Fixed shortcuts in Find Rooms and Edit Informarks dialogs
 - Refactored Edit Informarks dialog to make more sense

## MMapper 2.5.1 (August 7, 2018)

### Changes:
 - Added new Stable flag for rooms
 - Integrated client supports automatic character set negotiation (RFC 2066)
 - Fixed bug that caused unnecessary NO_MATCH exits to be applied to maps
 - Squished bug that prevented auto-mapping sundeath for rooms to the west of the player
 - Exits now preserve their color from MUME

## MMapper 2.5.0 (May 13, 2018)

### Changes:
 - MMapper now encrypts your connection to MUME using TLS
 - Holding control and using the mouse wheel navigates layers
 - Improved the right click context menu
 - MMapper's clock can now parse the new MUME time format
 - Group manager hosts can opt to not share themselves with clients
 - Exposed option to force Software OpenGL on Windows and Linux
 - Integrated mud client can now save logs
 - Integrated mud client will not resize the terminal on window resize
 - Remote edit is now be passed through when disabled
 - Improved syncing when boarding/leaving the Grey Havens ferry

## MMapper 2.4.5 (February 11, 2018)

### Changes:
 - MMapper has a simple integrated mud client now
 - Remote editing is now supported through MMapper
 - Config Dialog has been overhauled
 - Updated MUME clock because the Third Age has been reset for the 10th time
 - Group Manager members are now displayed in sorted order

## MMapper 2.4.4 (January 28, 2018)

### Changes:
 - Fixed a critical storage bug that was corrupting InfoMarks. Please load a
   pre-2.4.3 map to recover your data. Sorry :(
 - Fixed regression that broke Powwow #prompt detection
 - Squished minor regression where light rooms could incorrectly use "dark"
   prompt information
 - Hidden exit flags are now optionally displayed after exits
 - Notes are now optionally displayed after exits
 - The Find Rooms dialog along with the "_search" and "_dirs" commands are now
   able to search through dynamic descriptions
 - Fixed layer navigation shortcuts on Mac OS X
 - Patched Group Manager crash on Mac
 - "damage", "fall", and "guarded" exit flags have been introduced for mappers
 - "clock" and "mail" load flags have been introduced for mappers
 - Pixmaps were improved for guilds, boats, food, treasure, and armour
 - Prompt text now influences group manager numerical scores
 - Contextual menus and cursors have been introduced to the map canvas
 - The parser will use the same newline terminator that the mud client is using

## MMapper 2.4.3 (January 16, 2018)

### Note:
 - Maps saved with 2.4.3 are not compatible with previous versions

### Changes:
 - Fixed bug where NO_RIDE rooms did not have a dark red cross
 - Fixed 7 year old bug that prevented renaming characters
 - Fix Ainur segfault that occurs in rooms with no exits
 - Group Manager is now a multithreaded component
 - QtIOCompressor is now optional for 2.4.3 saves
 - Fixed warning "waitForDisconnected() is not allowed in UnconnectedState"
 - Fixed typos where \n\r should have been \r\n
 - The default "cha charset" setting is now Latin-1

## MMapper 2.4.2 (January 1, 2018)

### Changes:
 - Fixed critical bug that prevented secret door names from being displayed
 - The last folder you open a map in will be remembered
 - The first time you open MMapper it will try to load a map automatically
 - Mume Clock will now show you the time even if it isn't certain
 - Fixed a 4 year old bug where no-ride rooms were not red

---
## MMapper 2.4.1 (December 30, 2017)

### Changes:
 - Resolved issues with QtIOCompressor library bundling by building it directly into MMapper
 - Group Manager will only attempt to connect for up to 5 seconds before timing out
 - MMapper's built in MumeClock will now only sync with known weather strings
 - First attempt at getting Ainur exits and <snoop> tags working

## MMapper 2.4.0 (December 26, 2017)

### Note:
  - The .mm2 file format changed. Old files can be read, but files saved
    with this version can't be used with MMapper 2.3.x or older.

### Changes:
  - Added support for classes of info marks, with different colors and font decorations (teoli)
  - Added ability to rotate info marks (teoli)
  - Fixed wall color when a special exit was present (was white instead of black) (teoli)
  - Added ability to store direction of river flows and display it on the map (teoli)
  - Added no_flee exit flag (nschimme)
  - More exit flags are now shown on the UI (nschimme)
  - Added support for trolls to automatically map rooms that cause sundeath (nschimme)
  - Added built in mume clock (nschimme)
  - Distant player's location is now hinted (nschimme)
  - Anti-aliasing and trilinear filtering can now be enabled (nschimme)
  - Configuration panel has a new path machine tab (nschimme)
  - Custom background colors can now be selected (nschimme)
  - Emulated prompts now show the current terrain and lighting (nschimme)
  - Web maps can now be exported (waba)
  - MMapper only support XML mode now and uses gratuitous flags (nschimme)
  - Added new `_search` and `_dirs` commands to find rooms and their paths (ethorondil)
  - Notes can now be printed from the command line (ethorondil)
  - GNOME and KDE integration improved (kalev)
  - Menus standardized and first time use improved (nschimme)

---
## MMapper 2.3.6 (December 9, 2015)

### Changes:
  - High DPI displays are now supported such as Retina displays (nschimme)

## MMapper 2.3.5 (July 29, 2015)

### Changes:
  - Fixed bug that prevented connections from having TCP KeepAlive (nschimme)
  - Updated base map to include new zones; thanks Ortansia! (nschimme)

## MMapper 2.3.4 (May 1, 2015)

### Changes:
  - All connections now utilize TCP KeepAlive to help with dropped connections (nschimme)
  - Prompts are now correctly identified and remembered for internal commands (nschimme)
  - Cleaned up vote and Windows build code (nschimme)

## MMapper 2.3.3 (January 18, 2015)

### Changes:
  - [GroupManager] Player's hp, mana, and moves are now correctly updated (nschimme)
  - [GroupManager] Player's room name has been moved into the far right column (nschimme)
  - [GroupManager] Linux and Mac hosts can now accept incoming connections (nschimme)
  - Prompts should not be displayed after an internal command like `_help` is run (nschimme)
  - Added new `_vote` command and menu action to vote for MUME on TMC (nschimme)

## MMapper 2.3.2 (January 17, 2015)

### Changes:
 - Fixed critical bug that disallowed Mac and Linux users to connect to MMapper (nschimme)

## MMapper 2.3.1 (January 17, 2015)

### Changes:
 - Telnet characters now parsed correctly (nschimme)
 - Info marks load correctly (nschimme)
 - Updated base map with the new zones (nschimme)

## MMapper 2.3.0 (January 17, 2015)

### Changes:
 - Build now requires Qt5.2 and CMake 2.8.11 (nschimme)
 - Moved source to GitHub (nschimme)
 - Bug fixes and build improvements (kovis)
 - Property window can be resized (kovis)
 - Prompt detection fixes (kalev)
 - New note search feature (Arfang)

---
## MMapper 2.2.1 (July 14, 2013)

### Changes:
 - Build fixes
 - Fix issues with XML mode in the account menu
 - Make `_name` and `_noride` commands work (thanks Waba!)

## MMapper 2.2.0 (July 13, 2013)

### Changes:
 - Compatibility fixes with latest MUME server
 - Build fixes
 - Minor crasher fixes
 - Add links to MMapper and MUME related websites in Help menu
 - Help->About redesign
 - Portions of the code relicensed under GPLv2+ (was GPLv2 before)
 - Show the MMapper version on the splash screen
 - Use telnet sequences for detecting prompt
 - Automatically switch on the xml mode
 - Initial support for trails and climb exits
