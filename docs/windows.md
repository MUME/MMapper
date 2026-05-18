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
  This application is not digitally signed, which may cause Microsoft Edge or Windows SmartScreen to block it initially.<br>
  <strong>To download and run the application:</strong>
  <ol>
    <li><strong>If Edge blocks the download:</strong> Open your download list, click the <strong>...</strong> (three dots) next to the file, and select <strong>Keep</strong>. Then click <strong>Show more</strong> and select <strong>Keep anyway</strong>.</li>
    <li><strong>If SmartScreen blocks the installer:</strong> Click <strong>More info</strong> in the "Windows protected your PC" window, then click <strong>Run anyway</strong>.</li>
    <li><strong>If the installer still won't run:</strong> Right-click the downloaded <code>.exe</code> file and select <strong>Properties</strong>. In the <strong>General</strong> tab, check the <strong>Unblock</strong> checkbox at the bottom and click <strong>OK</strong>.</li>
  </ol>
  <small>You only need to do this the first time you run a new version.</small>
</div>

{% include download.md %}
