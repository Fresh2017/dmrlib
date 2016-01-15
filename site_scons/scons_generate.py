import re

RE_VAR = re.compile(r'@([^@]+)@')

def GenerateHeader(target, source, env):
	"""Generate a header file using the passed Environment."""
	for dst, src in zip(target, source):
		print('Generating {dst} from {src}'.format(dst=dst, src=src))
		config_t = open(str(src), 'r')
		template = config_t.read()
		config_t.close()

		variable = env.Dictionary()
		generate = template
		for key in RE_VAR.findall(template):
			generate = generate.replace('@' + key + '@', str(variable[key]))

		config_h = open(str(dst), 'w')
		config_h.write(generate)
		config_h.close()
