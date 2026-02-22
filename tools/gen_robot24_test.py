#!/usr/bin/env python3
"""Generate a Robot 24 SSTV test signal based on MMSSTV-aligned parameters."""
import struct, math, wave

SAMPLE_RATE = 48000

class PhaseOscillator:
    def __init__(self, sr=SAMPLE_RATE, amplitude=0.4):
        self.sr = sr
        self.amplitude = amplitude
        self.phase = 0.0
        self.samples = []

    def add_tone(self, freq, duration_ms):
        n = int(self.sr * duration_ms / 1000.0)
        for _ in range(n):
            v = self.amplitude * math.sin(self.phase)
            self.samples.append(int(v * 32767))
            self.phase += 2.0 * math.pi * freq / self.sr

    def add_freq_samples(self, freq, num_samples):
        for _ in range(num_samples):
            v = self.amplitude * math.sin(self.phase)
            self.samples.append(int(v * 32767))
            self.phase += 2.0 * math.pi * freq / self.sr

def freq_for_lum(value):
    return 1500.0 + (value / 255.0) * 800.0

def gen_robot24():
    osc = PhaseOscillator()

    # Calibration Leader
    osc.add_tone(1900, 300)
    osc.add_tone(1200, 10)
    osc.add_tone(1900, 300)

    # VIS Code for Robot 24 = 0x04 (sent LSB first with parity)
    # Robot 24 is 132 (0x84)
    vis_code = 0x84
    osc.add_tone(1200, 30) # Start bit
    parity = 0
    for bit in range(7):
        b = (vis_code >> bit) & 1
        parity ^= b
        freq = 1100.0 if b == 1 else 1300.0
        osc.add_tone(freq, 30)
    
    # Parity bit (even parity)
    freq = 1100.0 if parity == 1 else 1300.0
    osc.add_tone(freq, 30)
    
    osc.add_tone(1200, 30) # Stop bit

    # Robot 24 Parameters (from our update)
    # Total Line: 200ms
    SYNC_MS = 8.0
    PORCH_MS = 2.0
    GAP_MS = 4.0
    Y_MS = 92.0
    C_MS = 46.0
    
    LINES = 120
    PIXELS_Y = 160
    PIXELS_C = 160 # Robot 24 is 160x120

    samples_per_pixel_y = (Y_MS / 1000.0 * SAMPLE_RATE) / PIXELS_Y
    samples_per_pixel_c = (C_MS / 1000.0 * SAMPLE_RATE) / PIXELS_C

    bars = [
        (255, 255, 255), (255, 255, 0), (0, 255, 255), (0, 255, 0),
        (255, 0, 255), (255, 0, 0), (0, 0, 255), (0, 0, 0),
    ]
    bar_width = PIXELS_Y // len(bars)

    def add_scan(color_channel, num_pixels, spp):
        samples_done = 0
        for px in range(num_pixels):
            bar_idx = min(px // bar_width, len(bars) - 1)
            val = bars[bar_idx][color_channel]
            freq = freq_for_lum(val)
            target = int((px + 1) * spp)
            n = target - samples_done
            if n > 0:
                osc.add_freq_samples(freq, n)
                samples_done += n

    print("Generating Robot 24 Image data...")
    for y in range(LINES):
        # Sync (1200 Hz)
        osc.add_tone(1200, SYNC_MS)
        # Porch (1500 Hz)
        osc.add_tone(1500, PORCH_MS)
        
        # Y Scan (92ms)
        # Y = 0.3R + 0.59G + 0.11B
        def get_y(idx):
             r, g, b = bars[idx]
             return int(0.3*r + 0.59*g + 0.11*b)

        samples_done = 0
        for px in range(PIXELS_Y):
            bar_idx = min(px // bar_width, len(bars) - 1)
            freq = freq_for_lum(get_y(bar_idx))
            target = int((px + 1) * samples_per_pixel_y)
            n = target - samples_done
            if n > 0:
                osc.add_freq_samples(freq, n)
                samples_done += n
        
        # Gap 1 (1500 Hz)
        osc.add_tone(1500, GAP_MS)
        
        # R-Y (Cr) Scan (46ms)
        def get_cr(idx):
             r, g, b = bars[idx]
             y = 0.3*r + 0.59*g + 0.11*b
             return int((r - y) * 0.7 + 128) # Simple approx for test pattern

        samples_done = 0
        for px in range(PIXELS_C):
            bar_idx = min(px // bar_width, len(bars) - 1)
             # Cr bar index needs to handle width correctly
            freq = freq_for_lum(get_cr(bar_idx))
            target = int((px + 1) * samples_per_pixel_c)
            n = target - samples_done
            if n > 0:
                osc.add_freq_samples(freq, n)
                samples_done += n
        
        # Gap 2 (1500 Hz)
        osc.add_tone(1500, GAP_MS)

        # B-Y (Cb) Scan (46ms)
        def get_cb(idx):
             r, g, b = bars[idx]
             y = 0.3*r + 0.59*g + 0.11*b
             return int((b - y) * 0.56 + 128) # Simple approx for test pattern

        samples_done = 0
        for px in range(PIXELS_C):
            bar_idx = min(px // bar_width, len(bars) - 1)
            freq = freq_for_lum(get_cb(bar_idx))
            target = int((px + 1) * samples_per_pixel_c)
            n = target - samples_done
            if n > 0:
                osc.add_freq_samples(freq, n)
                samples_done += n

        # Porch (1500 Hz)
        osc.add_tone(1500, PORCH_MS)

    osc.add_tone(1900, 100)
    return osc.samples

if __name__ == "__main__":
    audio = gen_robot24()
    outfile = r"C:\Users\chico\Documents\robot24_test_aligned.wav"
    with wave.open(outfile, 'w') as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(SAMPLE_RATE)
        data = struct.pack(f'<{len(audio)}h', *audio)
        wf.writeframes(data)
    print(f"Generated {outfile}")
