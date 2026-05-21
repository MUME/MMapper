---
layout: default
title: Get MMapper for Linux
---

Choose your preferred distribution method below. We recommend using Flathub for a sandboxed environment and easy updates across most Linux distributions.

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
