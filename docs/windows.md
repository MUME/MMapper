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
  <strong style="color: #d9534f;">Important Notice for .exe Downloaders:</strong><br>
  This application is not digitally signed, so Windows might prevent it from running initially.<br>
  <strong>To run the application:</strong>
  <ol>
    <li>Right-click the downloaded <code>.exe</code> file and select "Properties".</li>
    <li>In the Properties window, look for a section near the bottom that says "Security".</li>
    <li>If you see a checkbox or button labeled "Unblock", check it or click it.</li>
    <li>Click "Apply" and then "OK".</li>
    <li>You should now be able to run the MMapper installer.</li>
  </ol>
  <small>You only need to do this the first time you run this version.</small>
</div>

{% include download.md %}
