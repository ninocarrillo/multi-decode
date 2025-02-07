import subprocess

for mu in range(1, 90, 1):
	
	subprocess.run(["./modem.exe", "t2_16000.wav", "7", str(mu/100)])
	