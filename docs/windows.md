---
layout: default
title: Download MMapper for Windows
---

## Download MMapper for Windows

<a href="https://apps.microsoft.com/detail/9p6f2b68rf7g?referrer=appbadge&mode=direct">
     <img src="https://get.microsoft.com/images/en-us%20dark.svg" width="200" style="vertical-align: middle;"/>
</a><span class="recommendation-text"> Recommended</span>

{% for asset in site.github.latest_release.assets %}
{% if asset.name contains 'sha256' %}
{% elsif asset.name contains 'exe' %}
<a href="{{ asset.browser_download_url }}" class="download-link">
    Download {{ asset.name }}
</a>
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

{% include download.md %}
