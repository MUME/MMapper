document.addEventListener('DOMContentLoaded', function() {
    let detectedArch = null;
    let isChromeOS = false;
    const downloadLinks = document.querySelectorAll('.download-link');

    // --- Architecture Detection (Best Effort) ---
    if (navigator.userAgentData && navigator.userAgentData.architecture) {
        detectedArch = navigator.userAgentData.architecture.toLowerCase();
    } else {
        const ua = navigator.userAgent.toLowerCase();
        if (ua.includes('x86_64') || ua.includes('amd64') || ua.includes('win64')) {
            detectedArch = 'x86_64';
        } else if (ua.includes('arm64') || ua.includes('aarch64')) {
            detectedArch = 'arm64';
        }
    }

    if (navigator.userAgentData && navigator.userAgentData.platform) {
        const platform = navigator.userAgentData.platform.toLowerCase();
        if (platform.includes('chromeos') || platform.includes('cros')) {
            isChromeOS = true;
        }
    }

    // --- Highlight Download Link ---
    downloadLinks.forEach(link => {
        const href = link.href.toLowerCase();

        // Only recommend .deb for ChromeOS
        if (isChromeOS && (href.includes('appimage') || href.includes('flatpak'))) {
            return;
        }

        let linkArch = null;
        if (href.includes('x86_64') || href.includes('amd64') || href.includes('x64')) {
            linkArch = 'x86_64';
        } else if (href.includes('arm64') || href.includes('aarch64')) {
            linkArch = 'arm64';
        }

        if (detectedArch && linkArch === detectedArch) {
            link.classList.add('recommended-download');
            const recommendation = document.createElement('span');
            recommendation.textContent = ' Recommended';
            recommendation.classList.add('recommendation-text');
            link.parentNode.insertBefore(recommendation, link.nextSibling);
        }
    });
});
