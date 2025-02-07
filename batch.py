import subprocess

for mu in range(10, 90, 1):
	
	subprocess.run(["./modem.exe", "t2_8000.wav", "7", str(mu/100)])
	