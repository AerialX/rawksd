#!/usr/bin/env python

# Kamek - build tool for custom C++ code in New Super Mario Bros. Wii
# All rights reserved (c) Treeki 2010
# Some function definitions by megazig

# Requires PyYAML

version_str = 'Kamek 0.1 by Treeki'

import binascii
import os
import os.path
import shutil
import struct
import subprocess
import sys
import tempfile
import yaml

import hooks

u32 = struct.Struct('>I')

verbose = True

def print_debug(s):
	if verbose: print('* '+s)


def read_configs(filename):
	with open(filename, 'r') as f:
		data = f.read()
	
	return yaml.safe_load(data)


current_unique_id = 0
def generate_unique_id():
	# this is used for temporary filenames, to ensure that .o files
	# do not overwrite each other
	global current_unique_id
	current_unique_id += 1
	return current_unique_id


def generate_riiv_mempatch(offset, data):
	return '<memory offset="0x%08X" value="%s" />' % (offset, binascii.hexlify(data))


def generate_ocarina_patch(offset, data):
	out = []
	count = len(data)
	
	offset -= 0x80000000
	for i in range(count >> 2):
		out.append('%08X %s' % (offset | 0x4000000, binascii.hexlify(data[i*4:i*4+4])))
		offset += 4
	
	return '\n'.join(out)



class KamekModule(object):
	_requiredFields = ['source_files']
	
	
	def __init__(self, filename):
		# load the module data
		self.modulePath = os.path.normpath(filename)
		self.moduleName = os.path.basename(self.modulePath)
		self.moduleDir = os.path.dirname(self.modulePath)
		
		with open(self.modulePath, 'r') as f:
			self.rawData = f.read()
		
		self.data = yaml.safe_load(self.rawData)
		if not isinstance(self.data, dict):
			raise(ValueError, 'the module file %s is an invalid format (it should be a YAML mapping)' % self.moduleName)
		
		# verify it
		for field in self._requiredFields:
			if field not in self.data:
				raise(ValueError, 'Missing field in the module file %s: %s' % (self.moduleName, field))



class KamekBuilder(object):
	def __init__(self, project, configs):
		self.project = project
		self.configs = configs
	
	
	def build(self):
		print_debug('Starting build')
		
		self._prepare_dirs()
		
		for config in self.configs:
			self._set_config(config)
			
			self._configTempDir = tempfile.mkdtemp()
			print_debug('Temp files for this configuration are in: '+self._configTempDir)
			
			self._patches = []
			self._rel_patches = []
			self._hooks = []
			
			# hook setup
			self._hook_contexts = {}
			for name, hookType in hooks.HookTypes.items():
				if hookType.has_context:
					self._hook_contexts[hookType] = hookType.context_type()
			
			self._create_hooks()
			self._compile_modules()
			self._link()
			self._read_symbol_map()
			
			for hook in self._hooks:
				hook.create_patches()
			
			self._create_patch()
			
			shutil.rmtree(self._configTempDir)
	
	
	def _prepare_dirs(self):
		self._outDir = self.project.makeRelativePath(self.project.data['output_dir'])
		print_debug('Project will be built in: '+self._outDir)
		
		if not os.path.isdir(self._outDir):
			os.makedirs(self._outDir)
			print_debug('Created that directory')
	
	
	def _set_config(self, config):
		self._config = config
		print_debug('---')
		print_debug('Building for configuration: '+config['friendly_name'])
		
		self._config_short_name = config['short_name']
		self._rel_area = (config['rel_area_start'], config['rel_area_end'])
	
	
	def _create_hooks(self):
		print_debug('---')
		print_debug('Creating hooks')
		
		for m in self.project.modules:
			if 'hooks' in m.data:
				for hookData in m.data['hooks']:
					assert 'name' in hookData and 'type' in hookData
					
					print_debug('Hook: %s : %s' % (m.moduleName, hookData['name']))
					
					if hookData['type'] in hooks.HookTypes:
						hookType = hooks.HookTypes[hookData['type']]
						hook = hookType(self, m, hookData)
						self._hooks.append(hook)
					else:
						raise(ValueError, 'Unknown hook type: %s' % hookData['type'])
	
	
	def _compile_modules(self):
		print_debug('---')
		print_debug('Compiling modules')
		
		cc_command = ['powerpc-eabi-g++', '-nodefaultlibs', '-fno-builtin', '-fno-rtti', '--std=gnu++0x', '-Os', '-Wall', '-fno-exceptions', '-I' + os.environ['DEVKITPRO'] + '/libogc/include']
		for d in self._config['defines']:
			cc_command.append('-D%s' % d)
		for d in self._config['includes']:
			cc_command.append('-I%s' % d)
		
		self._moduleFiles = []
		for m in self.project.modules:
			for sourcefile in m.data['source_files']:
				print_debug('Compiling %s : %s' % (m.moduleName, sourcefile))
				
				objfile = os.path.join(self._configTempDir, '%d.o' % generate_unique_id())
				new_command = cc_command + ['-c', '-o', objfile, sourcefile]
				
				errorVal = subprocess.call(new_command)
				if errorVal != 0:
					print('BUILD FAILED!')
					print('g++ returned %d - an error occurred while compiling %s' % (errorVal, sourcefile))
					sys.exit(1)
				
				self._moduleFiles.append(objfile)
		
		print_debug('Compilation complete')
	
	
	def _link(self):
		print_debug('---')
		print_debug('Linking project')
		
		self._mapFile = '%s/%s_linkmap.map' % (self._outDir, self._config_short_name)
		self._outFile = '%s/%s_out.bin' % (self._outDir, self._config_short_name)
		
		ld_command = ['powerpc-eabi-ld', '-L.', '-s']
		ld_command.append('-o')
		ld_command.append(self._outFile)
		ld_command.append('-Ttext')
		ld_command.append(self._config['address'])
		ld_command.append('-T')
		ld_command.append(self._config['linker_script'])
		ld_command.append('-Map')
		ld_command.append(self._mapFile)
		ld_command.append('--no-demangle') # for debugging
		ld_command += self._moduleFiles
		
		errorVal = subprocess.call(ld_command)
		if errorVal != 0:
			print('BUILD FAILED!')
			print('ld returned %d' % errorVal)
			sys.exit(1)
		
		print_debug('Linked successfully')
	
	
	def _read_symbol_map(self):
		print_debug('---')
		print_debug('Reading symbol map')
		
		self._symbols = []
		
		file = open(self._mapFile, 'r')
		
		for line in file:
			if '__text_start' in line:
				self._textSegStart = int(line.split()[0],0)
				break

		# now read the individual symbols
		# this is probably a bad method to parse it, but whatever
		for line in file:
			if '__text_end' in line:
				self._textSegEnd = int(line.split()[0],0)
				break
			
			if not line.startswith('                '): continue
			
			sym = line.split()
			sym[0] = int(sym[0],0)
			self._symbols.append(sym)
		
		# we've found __text_end, so now we should be at the output section
		currentEndAddress = self._textSegEnd
		currentEndAddress = 0
		
		for line in file:
			if line[0] == '.':
				# probably a segment
				data = line.split()
				if len(data) < 3: continue
				
				segAddr = int(data[1],0)
				segSize = int(data[2],0)
				if segAddr+segSize > currentEndAddress:
					currentEndAddress = segAddr+segSize
		
		self._codeStart = self._textSegStart
		self._codeEnd = currentEndAddress
		
		file.close()
		print_debug('Read, %d symbol(s) parsed' % len(self._symbols))
		
		
		# next up, run it through c++filt
		print_debug('Running c++filt')
		p = subprocess.Popen('powerpc-eabi-c++filt', stdin=subprocess.PIPE, stdout=subprocess.PIPE, encoding='utf-8')
		
		symbolNameList = [sym[1] for sym in self._symbols]
		filtResult = p.communicate('\n'.join(symbolNameList))
		filteredSymbols = filtResult[0].split('\n')
		
		for sym, filt in zip(self._symbols, filteredSymbols):
			sym.append(filt)
		
		print_debug('Done. All symbols complete.')
		print_debug('Generated code is at 0x%08X .. 0x%08X' % (self._codeStart, self._codeEnd - 4))
	
	
	def _find_func_by_symbol(self, find_symbol):
		for sym in self._symbols:
			if sym[2] == find_symbol:
				return sym[0]
		
		raise(ValueError, 'Cannot find function: %s' % find_symbol)
	
	
	def _add_patch(self, offset, data):
		# ONLY RELEVANT FOR NSMBW
		#if offset >= self._rel_area[0] and offset <= self._rel_area[1]:
		#	self._rel_patches.append((offset, data))
		#else:
			self._patches.append((offset, data))
	
	
	def _create_patch(self):
		print_debug('---')
		print_debug('Creating patch')
		
		# convert the .rel patches to KamekPatcher format
		if len(self._rel_patches) > 0:
			kamekpatch = ''
			for patch in self._rel_patches:
				if len(patch[1]) > 4:
					# block patch
					kamekpatch += u32.pack(len(patch[1]) / 4)
					kamekpatch += u32.pack(patch[0])
					kamekpatch += patch[1]
				else:
					# single patch
					kamekpatch += u32.pack(patch[0])
					kamekpatch += patch[1]
			
			kamekpatch += u32.pack(0xFFFFFFFF)
			self._patches.append((0x817F4800, kamekpatch))
		
		# add the outfile as a patch
		file = open(self._outFile, 'rb')
		patch = (self._codeStart, file.read())
		file.close()
		
		self._patches.append(patch)
		
		# generate a Riivolution patch
		riiv = open('%s/%s_riiv.xml' % (self._outDir, self._config['short_name']), 'w')
		for patch in self._patches:
			riiv.write(generate_riiv_mempatch(*patch) + '\n')
		
		riiv.close()
		
		# generate an Ocarina patch
		ocarina = open('%s/%s_ocarina.txt' % (self._outDir, self._config['short_name']), 'w')
		for patch in self._patches:
			ocarina.write(generate_ocarina_patch(*patch) + '\n')
		
		ocarina.close()
		
		print_debug('Patches generated')



class KamekProject(object):
	_requiredFields = ['output_dir', 'modules']
	
	
	def __init__(self, filename):
		# load the project data
		self.projectPath = os.path.abspath(filename)
		self.projectName = os.path.basename(self.projectPath)
		self.projectDir = os.path.dirname(self.projectPath)
		
		with open(self.projectPath, 'r') as f:
			self.rawData = f.read()
		
		self.data = yaml.safe_load(self.rawData)
		if not isinstance(self.data, dict):
			raise(ValueError, 'the project file is an invalid format (it should be a YAML mapping)')
		
		# verify it
		for field in self._requiredFields:
			if field not in self.data:
				raise(ValueError, 'Missing field in the project file: %s' % field)
		
		# load each module
		self.modules = []
		for moduleName in self.data['modules']:
			modulePath = self.makeRelativePath(moduleName)
			self.modules.append(KamekModule(modulePath))
	
	
	def makeRelativePath(self, path):
		return os.path.normpath(os.path.join(self.projectDir, path))
	
	
	def build(self):
		# compile everything in the project
		builder = KamekBuilder(self, self.configs)
		builder.build()



def main():
	print(version_str)
	print()
	
	if len(sys.argv) < 2:
		print('No input file specified')
		sys.exit()
	
	project = KamekProject(os.path.normpath(sys.argv[1]))
	project.configs = read_configs('kamek_configs.yaml')
	project.build()



if __name__ == '__main__':
	main()


