Download MMapper {{ site.github.latest_release.tag_name }}
=========
{% for asset in site.github.latest_release.assets %}{% if asset.name != 'arda.mm2' %}[![Download {{ asset.name }}](https://img.shields.io/github/downloads/{{ site.github.owner_name }}/{{ site.github.repository_name }}/latest/{{ asset.name }}.svg)]({{ asset.browser_download_url }} "Download {{ asset.name }}")  
{% endif %}{% endfor %}

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
Please set up your client according to this [guide](https://github.com/MUME/MMapper/wiki)

## Frequently Asked Questions
1.  [Troubleshooting](https://github.com/MUME/MMapper/wiki/Troubleshooting)
2.  What is [MUME](http://mume.org/mume.php)?

## Found a bug?
Please report it [here](https://github.com/MUME/MMapper/issues)
