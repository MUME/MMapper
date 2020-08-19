Download MMapper
=========
MMapper is a graphical client and mapper for a game named [Multi-Users in Middle-earth](https://mume.org).
The game is traditionally played in a text-only mode, but MMapper tries
to represent the virtual world in user-friendly graphical environment. It acts
as a proxy between a telnet client and a MUD server, being able to analyze game
data in real time and show player's position on a map.

### Windows
{% for asset in site.github.latest_release.assets %}
{% if asset.name == 'arda.mm2' or asset.name contains 'sha256' %}
{% elsif asset.name contains 'exe' %}
[![Download {{ asset.name }}](https://img.shields.io/github/downloads/{{ site.github.owner_name }}/{{ site.github.repository_name }}/latest/{{ asset.name }}.svg)]({{ asset.browser_download_url }} "Download {{ asset.name }}")
{% else %}
{% endif %}
{% endfor %}
### Mac
{% for asset in site.github.latest_release.assets %}
{% if asset.name == 'arda.mm2' or asset.name contains 'sha256' %}
{% elsif asset.name contains 'dmg' %}
[![Download {{ asset.name }}](https://img.shields.io/github/downloads/{{ site.github.owner_name }}/{{ site.github.repository_name }}/latest/{{ asset.name }}.svg)]({{ asset.browser_download_url }} "Download {{ asset.name }}")
{% else %}
{% endif %}
{% endfor %}
### Linux
{% for asset in site.github.latest_release.assets %}
{% if asset.name == 'arda.mm2' or asset.name contains 'sha256' %}
{% elsif asset.name contains 'AppImage' or asset.name contains 'deb' %}
[![Download {{ asset.name }}](https://img.shields.io/github/downloads/{{ site.github.owner_name }}/{{ site.github.repository_name }}/latest/{{ asset.name }}.svg)]({{ asset.browser_download_url }} "Download {{ asset.name }}")
{% else %}
{% endif %}
{% endfor %}

## News
Latest release {{ site.github.latest_release.tag_name }} ({{ site.github.latest_release.published_at | date_to_string }})

{{ site.github.latest_release.body }}

## Features
1.  Automatic room creation and connection of rooms during mapping
2.  Exit detection and note taking
3.  Fast 3D OpenGL rendering with pseudo 3D layers
4.  User friendly design with drag and drop mouse operations
5.  Multi-platform support for Windows, Mac, and Linux
6.  Multiplayer capabilities where group members can connect to your map

## Usage
Please set up your client according to this [guide](https://github.com/MUME/MMapper/wiki/Installing)

## Frequently Asked Questions
1.  [Troubleshooting](https://github.com/MUME/MMapper/wiki/Troubleshooting)
2.  What is [MUME](https://mume.org)?

## Found a bug?
Please report it [here](https://github.com/MUME/MMapper/issues)
