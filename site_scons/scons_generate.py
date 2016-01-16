import os
import re
import subprocess

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


def GenerateVersions(env, versions):
	"""Parse software versions from a file.

	On each line, you will write:

		<name>=<major>.<minor>.<patch>

	"""
	tag = ''
	patch_git = None
	if os.path.isdir('.git'):
	    GIT_VERSION = subprocess.check_output([
	        'git', 'describe', '--long',
	    ]).strip().split('-')

	    tag = 'git'
	    patch_git = '-'.join(GIT_VERSION[-2:])

	with open(versions) as handle:
		for line in handle:
			name, version = line.strip().split('=')
			major, minor, patch = version.strip().split('.')
			update = {
				name.upper() + '_VERSION_MAJOR': major,
				name.upper() + '_VERSION_MINOR': minor,
				name.upper() + '_VERSION_PATCH': patch_git or patch,
				name.upper() + '_VERSION_TAG': tag,
			}
			env.Append(**update)

	print(env.Dump())
