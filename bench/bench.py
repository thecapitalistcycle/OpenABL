# Copyright 2017 OpenABL Contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import re
import os
import subprocess
import sys

main_dir = os.path.dirname(os.path.realpath(__file__)) + '/..'
asset_dir = main_dir + '/asset'
example_dir = main_dir + '/examples'
try_openabl_bins = [main_dir + '/OpenABL', main_dir + '/build/OpenABL']

result_dir = os.environ.get('RESULT_DIR')
if result_dir is not None:
    if not os.path.isdir(result_dir):
        print('Result directory ' + result_dir + ' does not exist')
        sys.exit(1)
else:
    print('WARNING: No RESULT_DIR specified')

openabl_bin = None
for try_openabl_bin in try_openabl_bins:
    if os.path.isfile(try_openabl_bin):
        openabl_bin = try_openabl_bin
        break

if openabl_bin is None:
    print('OpenABL binary not found. Tried: ' + ', '.join(try_openabl_bins))
    sys.exit(1)

def openabl_run(model, backend, params, config):
    model_file = example_dir + '/' + model + '.abl'
    args = [
        openabl_bin,
        '-i', model_file,
        '-b', backend,
        '-A', asset_dir,
        '-R'
    ]

    for key, value in params.items():
        args.append('-P')
        args.append(key + '=' + str(value))

    for key, value in config.items():
        args.append('-C')
        args.append(key + '=' + str(value))

    try:
        return subprocess.check_output(args, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as err:
        print('Invocation of command\n' + ' '.join(args) + '\n'
                + 'exited with exit code ' + str(err.returncode)
                + ' and the following output:\n' + err.output)
        sys.exit(1)

def openabl_get_exec_time(model, backend, params, config):
    output = openabl_run(model, backend, params, config).decode('utf-8')
    matches = re.search('Execution time: (.*)s', output)
    if matches is None:
        raise RuntimeError('Failed to extract execution time')
    return float(matches.group(1))

def next_pow2(n):
    return 2**(n-1).bit_length()

def run_bench(backend, model, num_agents_range):
    (min_num_agents, max_num_agents) = num_agents_range
    num_timesteps = 100
    num_agents_factor = 2

    result = "n,t\n"
    print('Running %s on %s backend with %d-%d agents' %
        (model, backend, min_num_agents, max_num_agents))

    num_agents = min_num_agents
    while num_agents <= max_num_agents:
        params = {
            'num_timesteps': num_timesteps,
            'num_agents': num_agents,
        }

        # TODO Find a better solution for this
        config = {}
        if backend == 'flamegpu':
            config['flamegpu.buffer_size'] = next_pow2(num_agents)

        exec_time = openabl_get_exec_time(model, backend, params, config)
        csv_str = str(num_agents) + ',' + str(exec_time)

        result += csv_str + "\n"
        print(csv_str)

        num_agents *= num_agents_factor

    if result_dir is not None:
        file_name = result_dir + '/bench_' + model + '_' + backend + '.txt'
        with open(file_name, 'w') as f:
            f.write(result)

if len(sys.argv) < 2:
    print('''
Usage:
    [RESULT_DIR=data/]
    python bench.py \\
        backend \\
        [model1,model2,...] \\
        [min_agents-max_agents]

    Default models:
        * circle
        * boids2d
        * game_of_life
        * sugarscape
        * ants
        * predator_prey

    Default agent ranges:
        * c:        250-32000
        * mason:    250-128000
        * mason2:   250-128000
        * flame:    250-4000
        * flamegpu: 250-10240000
''')
    sys.exit(1)

backend = sys.argv[1]
if backend == 'flamegpu' and 'SMS' not in os.environ:
    print('Using flamegpu backend without SMS environment variable')
    sys.exit(1)

if len(sys.argv) >= 3:
    models = sys.argv[2].split(',')
else:
    models = [
        'circle', 'boids2d', 'game_of_life',
        'sugarscape', 'ants', 'predator_prey'
    ]

default_agent_ranges = {
    'c': (250, 32000),
    'mason': (250, 128000),
    'mason2': (250, 128000),
    'flame': (250, 4000),
    'flamegpu': (250, 10240000),
}

if len(sys.argv) >= 4:
    num_agents_spec = sys.argv[3].split('-')
    if len(num_agents_spec) != 2:
        print('Invalid argent number specification (min-max)')
        sys.exit(1)

    num_agents_range = (int(num_agents_spec[0]), int(num_agents_spec[1]))
else:
    num_agents_range = default_agent_ranges[backend]

for model in models:
    run_bench(backend, model, num_agents_range)
