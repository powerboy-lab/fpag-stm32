function spectrum_debug(input_path, fs_hz)
%SPECTRUM_DEBUG Compare a captured frame with MATLAB FFT.
%   spectrum_debug() uses a built-in test tone.
%   spectrum_debug("capture.csv", 48000000) loads one column of samples.

if nargin < 2 || isempty(fs_hz)
    fs_hz = 48e6;
end

if nargin < 1 || isempty(input_path)
    n = 1024;
    t = (0:n-1)' / fs_hz;
    x = 0.8*sin(2*pi*1.25e6*t) + 0.08*sin(2*pi*3.75e6*t) + 0.01*randn(size(t));
else
    x = readmatrix(input_path);
    x = x(:,1);
    n = numel(x);
end

nfft = 1024;
x = x(:);
if numel(x) < nfft
    x = [x; zeros(nfft-numel(x),1)];
else
    x = x(1:nfft);
end

w = hann(nfft, "periodic");
xw = x .* w;
cg = mean(w);

X = fft(xw, nfft);
mag = abs(X(1:nfft/2+1)) / nfft / cg * 2;
mag(1) = abs(X(1)) / nfft / cg;
if mod(nfft,2) == 0
    mag(end) = abs(X(nfft/2+1)) / nfft / cg;
end
f = (0:nfft/2)' * fs_hz / nfft;

[peak_mag, idx] = max(mag(2:end));
idx = idx + 1;
fprintf('Fs = %.0f Hz\n', fs_hz);
fprintf('Peak = %.2f Hz, Mag = %.6f\n', f(idx), peak_mag);

figure('Color','w');
subplot(2,1,1);
plot((0:numel(x)-1)/fs_hz, x, 'LineWidth', 1);
grid on;
xlabel('Time (s)');
ylabel('Amplitude');
title('Time Domain');

subplot(2,1,2);
plot(f, 20*log10(max(mag, eps)), 'LineWidth', 1);
grid on;
xlabel('Frequency (Hz)');
ylabel('Magnitude (dB)');
title('Spectrum');
end
