---
layout: default
title: Download MMapper for macOS
---

## Download MMapper for macOS
{% for asset in site.github.latest_release.assets %}
{% if asset.name contains 'sha256' %}
{% elsif asset.name contains 'dmg' %}
<a href="{{ asset.browser_download_url }}" class="download-link">
    Download {{ asset.name }}
</a>{% assign asset_name_lower = asset.name | downcase %}{% capture arch_label %}{% if asset_name_lower contains 'arm64' %}Apple Silicon{% elsif asset_name_lower contains 'x86_64' %}Intel{% endif %}{% endcapture %}{% if arch_label != "" %} <span class="arch-label">{{ arch_label }}</span>{% endif %}
{% endif %}
{% endfor %}

<div class="notice-box" id="mac-notice">
  <strong style="color: #d9534f;">Important Notice for macOS Users:</strong><br>
  This application is not notarized by Apple, so macOS will prevent it from opening directly.<br>
  <strong>To run the application the first time:</strong>
  <ol>
    <li>Drag the MMapper application to your <strong>Applications</strong> folder.</li>
    <li>Try to open MMapper from your Applications folder. When a warning appears, click <strong>Done</strong> or <strong>Cancel</strong> (this registers the app with macOS).</li>
    <li>Go to <strong>System Settings > Privacy & Security</strong> and scroll down to the <strong>Security</strong> section.</li>
    <li>Look for the message "MMapper was blocked..." and click the <strong>Open Anyway</strong> button.</li>
    <li>Enter your password if prompted, then click <strong>Open</strong> on the final dialog box.</li>
  </ol>
  <small>You only need to do this the first time you run a new version of MMapper.</small>
</div>

{% include download.md %}
