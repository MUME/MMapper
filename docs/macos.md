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
</a>
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

## Usage
Please set up your client according to this [guide](https://github.com/MUME/MMapper/wiki/Installing).

## Frequently Asked Questions
1.  [Troubleshooting](https://github.com/MUME/MMapper/wiki/Troubleshooting)
2.  What is [MUME](https://mume.org)?

## Found a bug?
Please report it [here](https://github.com/MUME/MMapper/issues).
