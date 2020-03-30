Download MMapper
=========
MMapper is a graphical mapper for a MUD named MUME (Multi-Users in Middle
Earth). The game is traditionally played in a text-only mode, but MMapper tries
to represent the virtual world in user-friendly graphical environment. It acts
as a proxy between a telnet client and a MUD server, being able to analyze game
data in real time and show player's position in a map.

{% for asset in site.github.latest_release.assets %}
{% if asset.name == 'arda.mm2' or asset.name contains 'sha256' %}
{% else %}
[![Download {{ asset.name }}](https://img.shields.io/github/downloads/{{ site.github.owner_name }}/{{ site.github.repository_name }}/latest/{{ asset.name }}.svg)]({{ asset.browser_download_url }} "Download {{ asset.name }}")
{% endif %}
{% endfor %}

## News
Latest release {{ site.github.latest_release.tag_name }} ({{ site.github.latest_release.published_at | date_to_string }})

{{ site.github.latest_release.body }}

## Features
1.  Automatic room creation during mapping
2.  Automatic connection of new rooms
3.  Terrain detection (forest, road, mountain, etc)
4.  Exits detections
5.  Fast OpenGL rendering
6.  Pseudo 3D layers and drag and drop mouse operations
7.  Multi platform support
8.  Group manager support to see other people on your map
9.  Integrated mud client
10.  Remote editing support

## Usage
Please set up your client according to this [guide](https://github.com/MUME/MMapper/wiki/Installing)

## Frequently Asked Questions
1.  [Troubleshooting](https://github.com/MUME/MMapper/wiki/Troubleshooting)
2.  What is [MUME](http://mume.org/mume.php)?

## Found a bug?
Please report it [here](https://github.com/MUME/MMapper/issues)
