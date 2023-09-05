import numpy as np
from tkinter import filedialog
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit
from scipy.signal import find_peaks
from scipy.stats import linregress

resolution_enhancement = 2

def generate_calibration(channel, known_lines):
    input_file = filedialog.askopenfilename(title=f"Select Channel {channel} Calibration", filetypes=[("Centroids","*.singles.csv")])
    if(len(input_file) < 1):
        print("Input file not provided, skipping...")
        return
    
    # read data once to figure out the y-value of the line
    hist = np.zeros((256, 256))
    pixelsize = 55e-6

    with open(input_file, 'r') as f:
        f.readline() # ignore header line
        
        for line in f:
            (x,y) = line.split(',')
            (x,y) = (float(x), float(y[:-1]))
            (x,y) = (y,x) # rotate image by 90 degrees
            (binx, biny) = (int(x/pixelsize), int(y/pixelsize))
            hist[binx, biny] += 1

    def fit_func(x, x0, ymax, sigma, c):
        return ymax*np.exp(-((x-x0)/sigma)**2)+c

    y_margin = np.sum(hist, axis=1)
    x = np.arange(len(y_margin))

    ymax_guess = max(y_margin)
    x0_guess = np.argwhere(y_margin == ymax_guess)[0][0]
    sigma_guess = 1
    c_guess = np.median(y_margin)

    params, cov = curve_fit(fit_func, x, y_margin, p0=(x0_guess, ymax_guess, sigma_guess, c_guess))

    x0 = params[0]
    sigma = params[2]
    width = 2*sigma

    # read data a second time, throwing out all points not along this line
    x_hist = np.zeros(256*resolution_enhancement)
    with open(input_file, 'r') as f:
        f.readline() # ignore header line
        
        for line in f:
            (x,y) = line.split(',')
            (x,y) = (float(x), float(y[:-1]))
            (x,y) = (y,x) # rotate image by 90 degrees
            (binx, biny) = (int(x/pixelsize), int(y/pixelsize*resolution_enhancement))
            if (binx < x0 - width) or (binx > x0 + width):
                continue
            
            x_hist[biny] += 1

    peaks, _ = find_peaks(x_hist, height=(max(x_hist)/15))
    print(f"Peaks at {peaks/resolution_enhancement}")

    if len(peaks) != len(known_lines):
        print(f"Found {len(peaks)} peaks, expected {len(known_lines)}; unable to match to known emission lines")
        plt.plot(np.arange(len(x_hist))/resolution_enhancement, x_hist)
        for p in peaks:
            plt.plot(p/resolution_enhancement, x_hist[p], 'bx')
        plt.xlabel("Camera pixel")
        plt.ylabel("Intensity")
        plt.title(f"Channel {channel} Calibration Spectrum")
        plt.show()
        return

    linfit = linregress(peaks/resolution_enhancement, known_lines)
    print(f"Calibration for channel {channel}:")
    print(f"Slope: {linfit.slope} (+/- {linfit.stderr/linfit.slope*100}%)")
    print(f"Intercept: {linfit.intercept} (+/- {linfit.intercept_stderr/linfit.intercept*100}%)")

    ends = np.array([0, 255])

    plt.plot(ends, ends*linfit.slope + linfit.intercept, 'b-')
    plt.plot(peaks/resolution_enhancement, known_lines, 'rx')
    plt.xlabel("Peak position (pixels)")
    plt.ylabel("Ar Emission Wavelength")
    plt.title(f"Channel {channel} Calibration")
    plt.show()

    return peaks/resolution_enhancement

if __name__ == '__main__':
    print("Calibration to Argon emission lines")
    Ar_lines = [794.8176, 800.6157, 801.4786, 810.3693, 811.5311, 826.4522, 840.821, 842.4648]
    print(f"Known lines: {Ar_lines}")
    peaks1 = generate_calibration(1, Ar_lines)
    peaks2 = generate_calibration(2, Ar_lines)

    if not (peaks1 is None or peaks2 is None):
        output_file = filedialog.asksaveasfilename(title=f"Select Output File", filetypes=[("Calibration Files", "*.calib.csv")])
        if(len(output_file) > 0):
            if('.' not in output_file):
                output_file = output_file + ".calib.csv"
            
            with open(output_file, 'w') as f:
                f.write("Wavelength [nm], Bin 1 [px], Bin 2 [px]\n")
                for ix in range(len(Ar_lines)):
                    f.write(f"{Ar_lines[ix]}, {peaks1[ix]}, {peaks2[ix]}\n")
        else:
            print("Output file not provided, skipping...")
