import subprocess
import json
import time
import argparse
import re
import os
import sys
import base64
from http.server import SimpleHTTPRequestHandler
from socketserver import ThreadingMixIn, TCPServer
import threading as thr
import signal
import shutil
import numpy as np
#import tools


stop_gen_event = thr.Event()
generating = thr.Event()
failed_img = ["./failed_gen.png"]

all_parameters = {}

received_params = {}

def get_python_executable():
    python_exec = sys.executable
    if python_exec and shutil.which(python_exec):
        return python_exec

    for python_cmd in ["python3", "python"]:
        if shutil.which(python_cmd):
            return python_cmd

    raise RuntimeError("Python not found!")
    
def divide_in_squares(list_c, xmin, xmax, ymin, ymax):
    """Recalculate boundaries based on grid divisions."""
    list_c = np.array(list_c) - [1, 1, 0]
    for col, line, n_squares in list_c:
        size_x = (xmax - xmin) / n_squares
        size_y = (ymax - ymin) / n_squares
        xmin, xmax = xmin + col * size_x, xmin + (col + 1) * size_x
        ymin, ymax = ymin + line * size_y, ymin + (line + 1) * size_y
    return xmin, xmax, ymin, ymax
    
python_exe = get_python_executable()


current_client_id = 0


def set_current_gen(id):
    global current_client_id
    current_client_id = id

def process_form_data(params, timeout):
    global all_parameters, stop_gen_event





    def is_url_or_path(value):
        url_regex = re.compile(
            r'^(?:http|ftp)s?://'  # http:// or https://
            r'(?:(?:[A-Z0-9](?:[A-Z0-9-]{0,61}[A-Z0-9])?\.)+(?:[A-Z]{2,6}\.?|[A-Z0-9-]{2,}\.?)|'  # Domain
            r'localhost|'  # localhost
            r'\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}|'  # IPv4
            r'?[A-F0-9]*:[A-F0-9:]+?)'  # IPv6
            r'(?::\d+)?'  # port
            r'(?:/?|[/?]\S+)$',  # path
            re.IGNORECASE
        )
        
        b64_regex = re.compile(
            r'^data:image/([a-zA-Z]+);base64,[A-Za-z0-9+/]+={0,2}$',  # Base64 images
            re.IGNORECASE
        )
        
        if re.match(b64_regex, value):
            return "b64"
        elif re.match(url_regex, value):
            return "url"
        elif os.path.isfile(value):
            return "image path"
        else:
            return "invalid"
        

    def download_image(url):
        import hashlib
        # Check if the value is a URL
        url_type = is_url_or_path(url)
        
        if url_type != "url" and url_type != "b64":
            return url  # If not a URL, return the original value
        elif url_type == "b64":
            
            header, image_data = url.split(",")
            header = header.split(";")[0].split(":")[1]
            ext = header.split("/")[-1]  # Extract the extension (e.g., png, jpeg)
            if ext not in ['png', 'jpg', 'jpeg']:
                ext = 'png'  # Fallback to PNG if unsupported format
            
            # Decode the Base64 string to get image data
            image_data = base64.b64decode(image_data)
            
            # Generate an MD5 hash for the image data
            md5_hash = hashlib.md5(image_data).hexdigest()
            
            file_name = f"./palettes/{md5_hash}.{ext}"
            
            # Check if the file already exists
            if os.path.isfile(file_name):
                return file_name
        
            # Save the image
            os.makedirs(os.path.dirname(file_name), exist_ok=True)
            with open(file_name, 'wb') as f:
                f.write(image_data)
            
            return file_name
        
        # Download the image
    

        import requests
        
        
        response = requests.get(url)
        if response.status_code == 200:
            # Calculate MD5 hash of the content
            md5_hash = hashlib.md5(response.content).hexdigest()
            # Determine the extension
            ext = url.split('.')[-1].lower()
            if ext not in ['png', 'jpg', 'jpeg']:
                ext = 'png'  # Fallback to PNG if unsupported format
            
            file_name = f"./palettes/{md5_hash}.{ext}"
            
            if os.path.isfile(file_name):
                return file_name 
            
            with open(file_name, 'wb') as file:
                file.write(response.content)
            
            return file_name
        else:
            raise Exception(f"Failed to download the image. Status Code: {response.status_code}")






    all_parameters['expression'] = params.get('expression', 'z*z+c')

    all_parameters['fractals'] = params.get("fractals", {
    'mandelbrot': False,
    'juliaset': False,
    'newton' : False,
    'newton_juliaset': False,
    'magnet': False,
    'lyapunov': False,
    'sandpile': False,
    })

    all_parameters['width'] = int(params.get('width', 1024))
    all_parameters['height'] = int(params.get('height', 1024))
    all_parameters['max_iter'] = int(params.get('max_iter', 400))
    all_parameters["escape_radius"] = float(params.get('escape_radius',0.0))
    all_parameters['top_colors'] = int(params.get('top_colors', 24))
    all_parameters['max_grains'] = int(params.get('max_grains', 3))
    all_parameters['juliaset_c_real'] = float(params.get('juliaset_c_real', -0.8))
    all_parameters['juliaset_c_imag'] = float(params.get('juliaset_c_imag', 0.16))

    all_parameters['use_palette'] = bool(params.get('use_palette', True))
    palette = params.get('palette', './palettes/palette.png')
    palette = download_image(palette)
    all_parameters['palette'] = palette
    all_parameters['gradient'] = int(params.get('gradient', 16))
    all_parameters['lake'] = bool(params.get('lake', True))
    lake_palette = params.get('lake_palette', './palettes/lake_palette.png')
    lake_palette = download_image(lake_palette)
    all_parameters['lake_palette'] = lake_palette
    
    all_parameters['shift_palette'] = int(params.get('shift_palette', 0))
    all_parameters['shift_palette_lake'] = int(params.get('shift_palette_lake',0))
    
    
    grid_length = int(params.get('grid_length', 3))
    column_aim = min(int(params.get('column_aim', 2)), grid_length)
    row_aim = min(int(params.get('row_aim', 2)), grid_length)

    coordinates = [[column_aim,row_aim,grid_length]]
    all_parameters['column_aim'] = column_aim
    all_parameters['row_aim'] = row_aim
    all_parameters['grid_length'] = grid_length
    all_parameters['coordinates'] = coordinates
    all_parameters['continue_aim'] = bool(params.get('continue_aim', False))

    all_parameters['quaternion_j'] = float(params.get('quaternion_j', 0.0))
    all_parameters['quaternion_k'] = float(params.get('quaternion_k', 0.0))

    if all_parameters['continue_aim'] and all_parameters['grid_length'] != 1:
        xmin, xmax, ymin, ymax = divide_in_squares(coordinates, all_parameters['xmin'], all_parameters['xmax'], all_parameters['ymin'], all_parameters['ymax'])
        all_parameters['xmin'], all_parameters['xmax'], all_parameters['ymin'], all_parameters['ymax'] = xmin, xmax, ymin, ymax
    else:
        all_parameters['xmin'] = float(params.get('xmin', -2.7))
        all_parameters['xmax'] = float(params.get('xmax', 2.7))
        all_parameters['ymin'] = float(params.get('ymin', -2.7))
        all_parameters['ymax'] = float(params.get('ymax', 2.7))

    print("\nYour coordinates: ", all_parameters['xmin'], all_parameters['xmax'], all_parameters['ymin'], all_parameters['ymax'], "\n")
    print(all_parameters['expression'].replace(" ", ""))

    all_parameters["z_initial_r"]= float(params.get('z_initial_r', 0.0))
    all_parameters["z_initial_i"]= float(params.get('z_initial_i', 0.0))
    all_parameters["newton_epsilon"]= float(params.get('newton_epsilon', 0.000001))
    all_parameters["sigma"]= float(params.get('sigma', 10.0))
    all_parameters["rho"]= float(params.get('rho', 28.0))
    all_parameters["beta"]= float(params.get('beta', 2.66666666))
    all_parameters["dt"]= float(params.get('dt', 0.01))
    all_parameters["rotation_angle"] = float(params.get('rotation_angle', 0.0))
    all_parameters["axis"] = int(params.get('axis', -1))
    all_parameters["max_point_size"] = int(params.get('max_point_size', 1))
    all_parameters["n_points"] = int(params.get('n_points',3))
    all_parameters["array_size"] = int(params.get('array_size',1))
    fractalize_image = (params.get('fractalize_image', False))
    fractalize_image = download_image(fractalize_image) if fractalize_image else False
    all_parameters['fractalize_image'] = fractalize_image
    all_parameters['fast_mode'] = bool(params.get('fast_mode', True))

    #zoom = False
    #max_zoom = 20

    
    if all_parameters['use_palette'] and not os.path.exists(palette):
        print(f"Palette file does not exist: {palette}")
        return failed_img
    if all_parameters['lake'] and not os.path.exists(lake_palette):
        print(f"Lake palette file does not exist: {lake_palette}")
        return failed_img


    
    
    json_data = json.dumps(all_parameters)
    
    
    process = subprocess.Popen(
        [python_exe, "runner.py", "-json_data", json_data],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    
    start_time = time.time()


    while True:
        if process.poll() is not None:
            print("Took: ",time.time() - start_time,"seconds")
            break
    
        if timeout > 0 and (time.time() - start_time) > timeout:
            process.kill()  
            print("Timeout")
            return failed_img
        elif stop_gen_event.is_set():
            process.kill()  
            print("Generation stopped")
            return failed_img
        time.sleep(0.2)
    

    

    stdout, stderr = process.communicate()
    
    print(stdout)
    

    if stderr:
        print(f"Error: {stderr}")
        
    

    if stdout:
        try:
            return json.loads(stdout.strip())
        except Exception as e:
            print(f"An Error Occurred: {e}")
            return failed_img
    
    return failed_img





def server(port, timeout):
    global stop_gen_event

    PORT = port
    DIRECTORY = ""

    class MyHttpRequestHandler(SimpleHTTPRequestHandler):

        def do_POST(self):
            global current_client_id

            content_type = self.headers.get('Content-Type')
            if content_type != 'application/json; charset=utf-8':
                self.send_response(400)
                self.end_headers()
                return

            content_length = int(self.headers['Content-Length'])
            body = self.rfile.read(content_length)
            try:
                received_params = json.loads(body.decode('utf-8'))
            except json.JSONDecodeError:
                self.send_response(400)
                self.end_headers()
                return

           
            if self.path == '/stop':
                tab_id = str(received_params.get('tabId', False))
    
                if received_params.get('action', False) == 'stop' and tab_id == current_client_id:
                    stop_gen_event.set()
                    self.send_response(200)
                    self.send_header('Content-Type', 'application/json; charset=utf-8')
                    self.end_headers()
                    self.wfile.write(json.dumps({'message': 'Stopping process...'}).encode('utf-8'))
                else:
                    self.send_response(400)
                    self.end_headers()
                return

            
            # Fractal generation request
            required_keys = ['width', 'height', 'max_iter', 'top_colors', 'max_grains', 'juliaset_c_real', 'juliaset_c_imag', 'xmin', 'xmax', 'ymin', 'ymax', 'palette', 'lake_palette']
            if all(key in received_params for key in required_keys) and not generating.is_set() :
                generating.set()
                stop_gen_event.clear()
                set_current_gen(str(received_params.get('tab_id', 0)))
                try:
                    fractal_result = ",".join(process_form_data(received_params, timeout))
                except Exception as e:
                    print(f"An Error Occurred: {e}")
                    fractal_result = ",".join(failed_img)
                self.send_response(200)
                self.send_header('Content-Type', 'application/json; charset=utf-8')
                self.end_headers()
                self.wfile.write(fractal_result.encode('utf-8'))
                generating.clear()
            elif generating.is_set():
                print("On queue...")
                while generating.is_set():
                    time.sleep(0.25)
                if all(key in received_params for key in required_keys):
                    generating.set()
                    stop_gen_event.clear()
                    set_current_gen(str(received_params.get('tab_id', 0)))
                    try:
                        fractal_result = ",".join(process_form_data(received_params, timeout))
                    except Exception as e:
                        print(f"An Error Occurred: {e}")
                        fractal_result = ",".join(failed_img)
                    self.send_response(200)
                    self.send_header('Content-Type', 'application/json; charset=utf-8')
                    self.end_headers()
                    self.wfile.write(fractal_result.encode('utf-8'))
                    generating.clear()
                else:
                    return
            else:
                self.send_response(400)
                self.end_headers()

        def translate_path(self, path):
            return SimpleHTTPRequestHandler.translate_path(self, f'/{DIRECTORY}{path}')

    class ThreadingHTTPServer(ThreadingMixIn, TCPServer):
        pass

    def signal_handler(signum, frame):
        print(f"Signal {signum} received. Shutting down.")
        sys.exit(0)

    signal.signal(signal.SIGTERM, signal_handler)

    Handler = MyHttpRequestHandler

    with ThreadingHTTPServer(("", PORT), Handler) as httpd:
        print(f"\nAnother Fractal Generator running at: http://localhost:{PORT}\n")
        httpd.serve_forever()



def main():
    global all_parameters
     
    parser = argparse.ArgumentParser(description='Process some arguments.')

    # Add the --noserver flag
    parser.add_argument(
        '-noserver',
        action='store_true',
        help='If specified, the server will not be started.'
    )
    parser.add_argument(
        '-port',
        type=int,
        default=8888,
        help='Port to serve. Defaults to 8888 if not specified.'
    )
    parser.add_argument(
        '-e',
        type=str,
        help='Expression to use "z^2+c".'
    )
    parser.add_argument(
        '-d',
        action='store_true',
        help='Don\'t save expressions on last_expressions.txt'
    )
    parser.add_argument(
        '-timeout',
        type=int,
        default=0,
        help='Time limit in seconds.'
    )
    args = parser.parse_args()
    all_parameters["save_expressions"] = not args.d
    


    result = subprocess.run(
        [python_exe, "runner.py", "-returndict"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    stdout = result.stdout
    # stderr = result.stderr
    # print(stderr )

    parameters = json.loads(stdout)


    all_parameters = parameters

        
    if args.noserver:
        if args.e:
            parameters['expression'] = str(args.e)


        # Let's Run
        process_form_data(parameters, args.timeout)

        # for i in range(36):
        #     all_parameters['expression'] = f"rotation(z*z+c,pi/{36-(i+1)}, 1k)" #(1/35)*i
        #     time.sleep(1)


        # n_coordinates is how many times it will use the array coordinates.
        #if all_parameters['video_out']:
            #imgs_to_video(all_parameters)

    else:

        server(args.port, args.timeout)


if __name__ == "__main__":
    main()
   
  