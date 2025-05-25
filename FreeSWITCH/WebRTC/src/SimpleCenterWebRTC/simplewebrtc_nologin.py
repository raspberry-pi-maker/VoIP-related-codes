from flask import Flask, render_template, redirect
import os
import ssl
import socket
    
app = Flask(__name__)

@app.route("/", methods = ["GET"])
def index():
    return render_template("index_webrtc.html", uri = hostname, me = "my_number")

@app.route("/webrtc", methods = ["GET"])
def index_webrtc():
    return render_template("index_webrtc.html", uri = hostname, me = "my_number")


if __name__ == '__main__':
    global hostname

    ssl_ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
    hostname = socket.gethostname() 
        
    cert_path = os.path.join("/etc/letsencrypt/live/", hostname)
    print (f"certificate path : {cert_path}")
    if os.path.exists(cert_path):   #use letsencrypt certificate
        full_chain = os.path.join(cert_path, "fullchain.pem")
        privkey = os.path.join(cert_path, "privkey.pem")
        ssl_ctx.load_cert_chain(full_chain, privkey)
        app.run(debug=True, host='0.0.0.0', port=5013, ssl_context = ssl_ctx, use_reloader=False) 
    else:    #cannot find Let's Encrypt certificates -> http
        print (f"Cannot find certificates ")
        app.run(debug=True, host='0.0.0.0', port=5013, use_reloader=False) 
    
    
