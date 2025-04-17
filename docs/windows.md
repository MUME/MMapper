---
layout: default
title: Download MMapper for Windows
---

## Download MMapper for Windows
{% for asset in site.github.latest_release.assets %}
{% if asset.name == 'arda.mm2' or asset.name contains 'sha256' %}
{% elsif asset.name contains 'exe' %}
<a href="{{ asset.browser_download_url }}" class="download-link">
    Download {{ asset.name }}
</a>
{% else %}
{% endif %}
{% endfor %}

<div class="notice-box" id="windows-notice">
  <strong style="color: #d9534f;">Important Notice for Windows Users:</strong><br>
  This application is not digitally signed. Windows Defender SmartScreen might prevent it from running initially.<br>
  <strong>To run the application:</strong>
  <ol>
    <li>Attempt to run the downloaded <code>.exe</code> file.</li>
    <li>If SmartScreen appears, click "More info".</li>
    <li>Then, click "Run anyway".</li>
  </ol>
  <small>You only need to do this the first time you run this version.</small>
</div>

## Usage
Please set up your client according to this [guide](https://github.com/MUME/MMapper/wiki/Installing).

## Frequently Asked Questions
1.  What <a href="about.html#changelog">new features, changes, or bug fixes are</a> in this version?
2.  I have [trouble and need help](https://github.com/MUME/MMapper/wiki/Troubleshooting) getting MMapper to run!
3.  What is [MUME](https://mume.org)?

## Found a bug?
Please report it [here](https://github.com/MUME/MMapper/issues).
