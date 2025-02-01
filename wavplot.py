# Python3
# Nino Carrillo
# 29 Mar 2024

import sys
from scipy.io.wavfile import read as readwav
from scipy.io.wavfile import write as writewav
import matplotlib.pyplot as plt


def main():
	# check correct version of Python
	if sys.version_info < (3, 0):
		print("Python version should be 3.x, exiting")
		sys.exit(1)
	# check correct number of parameters were passed to command line
	if len(sys.argv) != 2:
		print("Not enough arguments. Usage: python3 wavplot.py <sound file>")
		sys.exit(2)
	# try to open audio file
	try:
		input_sample_rate, input_audio = readwav(sys.argv[1])
	except:
		print('Unable to open audio file.')
		sys.exit(4)

	print(f"number of channels {input_audio.shape[1]}")

	
	plt.figure()
	plt.scatter(input_audio[:,0], input_audio[:,1])
	plt.xlim(-25000,25000)
	plt.ylim(-25000,25000)
	plt.show()
	
if __name__ == "__main__":
	main()