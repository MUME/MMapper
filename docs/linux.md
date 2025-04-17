---
layout: default
title: Download MMapper for Linux
---

## Download MMapper for Linux
{% for asset in site.github.latest_release.assets %}
{% if asset.name == 'arda.mm2' or asset.name contains 'sha256' or asset.name contains 'zsync' %}
{% elsif asset.name contains 'AppImage' or asset.name contains 'deb' or asset.name contains 'flatpak' %}
<a href="{{ asset.browser_download_url }}" class="download-link">
    Download {{ asset.name }}
</a>
{% else %}
{% endif %}
{% endfor %}

## Usage
Please set up your client according to this [guide](https://github.com/MUME/MMapper/wiki/Installing).

## Frequently Asked Questions
1.  What <a href="about.html#changelog">new features, changes, or bug fixes are</a> in this version?
2.  I have [trouble and need help](https://github.com/MUME/MMapper/wiki/Troubleshooting) getting MMapper to run!
3.  What is [MUME](https://mume.org)?

## Found a bug?
Please report it [here](https://github.com/MUME/MMapper/issues).
