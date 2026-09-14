document.addEventListener('DOMContentLoaded', function() {
    let detectedArch = null;
    let isChromeOS = false;
    const downloadLinks = document.querySelectorAll('.download-link, .platform-link');

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
    function addRecommendation(link) {
        link.classList.add('recommended-download');
        const recommendation = document.createElement('span');
        recommendation.textContent = ' Recommended';
        recommendation.classList.add('recommendation-text');

        // Place recommendation outside the link as per user preference
        if (link.classList.contains('platform-link')) {
            recommendation.classList.add('platform-recommendation');
        }
        link.parentNode.insertBefore(recommendation, link.nextSibling);
    }

    downloadLinks.forEach(link => {
        const href = link.href.toLowerCase();
        const platform = link.getAttribute('data-platform');

        // Do not recommend Windows .exe installers
        if (href.includes('.exe')) {
            return;
        }

        // For ChromeOS, only recommend the Web version
        if (isChromeOS) {
            if (platform === 'web') {
                addRecommendation(link);
            }
            return; // Don't recommend anything else on ChromeOS
        }

        let linkArch = null;
        if (href.includes('x86_64') || href.includes('amd64') || href.includes('x64')) {
            linkArch = 'x86_64';
        } else if (href.includes('arm64') || href.includes('aarch64')) {
            linkArch = 'arm64';
        }

        if (detectedArch && linkArch === detectedArch) {
            addRecommendation(link);
        }
    });

    // --- Touch / Mobile Device Detection for Keyboard Notice ---
    const isTouchDevice = ('ontouchstart' in window) || (navigator.maxTouchPoints > 0) || (navigator.msMaxTouchPoints > 0);
    const keyboardNotice = document.getElementById('keyboard-notice');
    if (keyboardNotice && isTouchDevice) {
        keyboardNotice.style.border = '2px solid #cc9933';
    }

    // --- PWA Instructions Platform Detection & Focus ---
    const ua = navigator.userAgent || '';
    const isIOS = /iPad|iPhone|iPod/.test(ua) || (navigator.platform === 'MacIntel' && navigator.maxTouchPoints > 1);
    const isAndroid = /Android/.test(ua);

    const iosBlock = document.getElementById('pwa-ios');
    const androidBlock = document.getElementById('pwa-android');

    if (isIOS && iosBlock && androidBlock) {
        // Move iOS block above Android block and highlight
        iosBlock.parentNode.insertBefore(iosBlock, androidBlock);
        iosBlock.style.borderLeft = '3px solid #5cb85c';
        iosBlock.style.paddingLeft = '0.5em';
    } else if (isAndroid && androidBlock && iosBlock) {
        // Highlight Android block
        androidBlock.style.borderLeft = '3px solid #5cb85c';
        androidBlock.style.paddingLeft = '0.5em';
    }
});
