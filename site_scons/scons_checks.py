def CheckCompileSource(context, name, source, compiler='.c'):
    context.Message('Checking for feature {0}... '.format(name))
    result = context.TryCompile(source, compiler)
    context.Result(result)
    return result

def CheckPKG(context, name):
	"""Check for a package via pkg-config."""
	context.Message('Checking for package {0}... '.format(name))
	result = context.TryAction("pkg-config --exists '{0}'".format(name))[0]
	context.Result(result)
	return result
