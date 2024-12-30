import matplotlib.pyplot as plt
import numpy as np
from scipy.optimize import curve_fit


def read_data_from_file(file_path):
    true_Vpp = []
    calculated_mW = []
    measured_mW = []

    with open(file_path, 'r') as file:
        lines = file.readlines()[2:]  # Skip the header lines

        for line in lines:
            if line.strip():  # Skip empty lines
                parts = line.split('|')
                true_Vpp.append(float(parts[1].strip()))
                calculated_mW.append(float(parts[2].strip()))
                measured_mW.append(float(parts[3].strip()))

    return true_Vpp, calculated_mW, measured_mW

true_Vpp, calculated_mW, measured_mW = read_data_from_file("50MHz-measurement.txt")


# Data 50Mhz
# true_Vpp = [3, 6.3, 15.3, 20, 23.5, 34.6, 43.3, 53]
# calculated_mW = [22.5, 99.22, 585.2, 1000, 1381, 2993, 4687, 7022]
# measured_mW = [23, 81, 472, 785, 1123, 2000, 3093, 4654]

# Define polynomial fitting functions
def fitting_function_5th(measured, a, b, c, d, e, f):
    return a * measured**5 + b * measured**4 + c * measured**3 + d * measured**2 + e * measured + f

# Convert measured_mW to a NumPy array
measured_mW = np.array(measured_mW)

# Fit the data
params_5th, _ = curve_fit(fitting_function_5th, measured_mW, calculated_mW)

# Extract the parameters
a5, b5, c5, d5, e5, f5 = params_5th

# Print the parameters to the console
print(f"5th-degree polynomial parameters: {{ {a5}, {b5}, {c5}, {d5}, {e5}, {f5} }}")

# Plot
plt.figure(figsize=(10, 6))
plt.plot(true_Vpp, calculated_mW, label='Calculated Power (mW)', marker='o')
plt.plot(true_Vpp, measured_mW, label='Measured Power (mW)', marker='s')
plt.plot(true_Vpp, fitting_function_5th(measured_mW, a5, b5, c5, d5, e5, f5), label='5th-degree Fit', marker='x')

# Labels and title
plt.xlabel('True Vpp')
plt.ylabel('Power (mW)')
plt.title('Calculated, Measured, and Fitted Power')
plt.legend()
plt.grid(True)

# Show the plot
plt.show()