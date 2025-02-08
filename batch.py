import subprocess

for mu in range(10, 30, 1):
	
	subprocess.run(["./modem.exe", "t1_16000.wav", "7", str(mu/1000)])
	