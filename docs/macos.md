---
layout: default
title: Download MMapper for macOS
---

## Download MMapper for macOS
{% for asset in site.github.latest_release.assets %}
{% if asset.name == 'arda.mm2' or asset.name contains 'sha256' %}
{% elsif asset.name contains 'dmg' %}
<a href="{{ asset.browser_download_url }}" class="download-link">
    Download {{ asset.name }}
</a>{% assign asset_name_lower = asset.name | downcase %}{% capture arch_label %}{% if asset_name_lower contains 'arm64' %}Apple Silicon{% elsif asset_name_lower contains 'x86_64' %}Intel{% endif %}{% endcapture %}{% if arch_label != "" %} <span class="arch-label">{{ arch_label }}</span>{% endif %}
{% else %}
{% endif %}
{% endfor %}

<div class="notice-box" id="mac-notice">
  <strong style="color: #d9534f;">Important Notice for macOS Users:</strong><br>
  This application is not notarized by Apple. macOS Gatekeeper might prevent it from opening directly.<br>
  <strong>To run the application the first time:</strong>
  <ol>
    <li>Locate the downloaded <code>.dmg</code> file and open it.</li>
    <li>Drag the MMapper application to your Applications folder (or another location).</li>
    <li>Right-click (or Control-click) the MMapper application icon.</li>
    <li>Select "Open" from the context menu.</li>
    <li>A dialog box will appear warning you about the developer. Click "Open" again.</li>
  </ol>
  <small>Alternatively, you can go to System Settings > Privacy & Security, scroll down to the "Security" section, and look for an "Open Anyway" button related to MMapper after trying to open it the first time.</small><br>
  <small>You only need to do this the first time you run this version.</small>
</div>

{% include download.md %}
