import subprocess
import sys


def main():
	if len(sys.argv) != 9:
		print("Not enough arguments. Usage: python3 batch.py <executable> <sound file> <taps> <start> <end> <step> <decoder type> <output file>")
		sys.exit(2)

	executable = sys.argv[1]
	sound_file = sys.argv[2]
	taps = int(sys.argv[3])
	start_mu = float(sys.argv[4])
	end_mu = float(sys.argv[5])
	step = float(sys.argv[6])
	decoder_type = int(sys.argv[7])
	output_file = sys.argv[8]

	start_mu = int(start_mu * 1000)
	end_mu = int(end_mu * 1000)
	step = int(step * 1000)
	for mu in range(start_mu, end_mu, step):
		
		subprocess.run([executable, sound_file, str(taps), str(mu/1000), str(decoder_type), output_file])

if __name__ == "__main__":
	main()