---
layout: default
title: Download MMapper for Linux
---

## Download MMapper for Linux

<a href='https://flathub.org/apps/org.mume.MMapper'>
    <img width='200' alt='Get it on Flathub' src='https://flathub.org/api/badge?locale=en' style="vertical-align: middle;"/>
</a>

[![Get it from the Snap Store](https://snapcraft.io/en/dark/install.svg)](https://snapcraft.io/mmapper)

{% for asset in site.github.latest_release.assets %}
{% if asset.name contains 'sha256' or asset.name contains 'zsync' %}
{% elsif asset.name contains 'AppImage' or asset.name contains 'deb' or asset.name contains 'flatpak' %}
<a href="{{ asset.browser_download_url }}" class="download-link">
    Download {{ asset.name }}
</a>
{% endif %}
{% endfor %}

{% include download.md %}
