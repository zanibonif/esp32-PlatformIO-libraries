#include "WebFileManager.h"
#include <LoggerHandler.h>
#include <LittleFSHandler.h>

static const char _HtmlPage[] PROGMEM = R"rawhtml(
<!DOCTYPE html><html><head>
<meta charset=UTF-8><meta name=viewport content="width=device-width,initial-scale=1">
<title>File Manager</title>
<style>
*{box-sizing:border-box}body{font-family:sans-serif;margin:0;background:#f0f2f5}
h2{margin:0;padding:12px 16px;background:#1565c0;color:#fff;font-size:1em;font-weight:500;letter-spacing:.3px}
#bc{display:flex;align-items:center;justify-content:space-between;padding:6px 16px;background:#fff;border-bottom:1px solid #ddd;font-size:.85em}
#sp{font-size:.8em;color:#666;white-space:nowrap;display:flex;align-items:center;gap:5px}
#w{margin:12px}
table{width:100%;border-collapse:collapse;background:#fff;border-radius:4px;box-shadow:0 1px 3px #0002;margin-bottom:10px}
th{padding:8px 10px;text-align:left;font-size:.76em;color:#999;border-bottom:1px solid #eee;text-transform:uppercase}
td{padding:7px 10px;font-size:.87em;border-bottom:1px solid #f2f2f2}
tr:last-child td{border:none}.er:hover td{background:#f5f7ff}
.c{background:#fff;border-radius:4px;padding:12px 14px;margin-top:10px;box-shadow:0 1px 3px #0002}
.c b{font-size:.8em;color:#666;display:block;margin-bottom:8px;text-transform:uppercase;letter-spacing:.3px}
input[type=text]{padding:4px 8px;border:1px solid #ddd;border-radius:3px;font-size:.87em;margin-right:6px}
input[type=text]:focus{border-color:#1565c0;outline:none}
.bp{display:inline-block;padding:4px 12px;border:none;border-radius:3px;cursor:pointer;background:#1565c0;color:#fff;font-size:.83em;vertical-align:middle}
.bp:hover{background:#0d47a1}.bp:disabled{background:#bbb;cursor:default}
.bs,.bd{display:inline-flex;align-items:center;justify-content:center;width:22px;height:22px;border-radius:3px;cursor:pointer;font-size:.82em;padding:0}
.bs{border:1px solid #bbb;background:#fff}
.bs:hover{background:#f0f0f0}
.bd{border:1px solid #e53935;background:none;color:#e53935;margin-left:3px}
.bd:hover{background:#fde8e8}
#st,#ms{margin-left:6px;font-size:.8em;color:#1565c0}
a{color:#1565c0;cursor:pointer;text-decoration:none}
.g{color:#aaa;font-size:.85em}
.dir td:first-child{font-weight:500}
</style></head>
<body>
<h2>File Manager</h2>
<div id=bc><span id=bl></span><span id=sp></span></div>
<div id=w>
<table><thead><tr><th>Name</th><th>Last modified</th><th>Size</th><th></th></tr></thead>
<tbody id=tb></tbody></table>
<div class=c><b>Upload file</b>
<label class=bp for=fi>Choose file</label><input type=file id=fi style=display:none onchange="document.getElementById('sf').textContent=this.files.length?this.files[0].name:'';document.getElementById('ub').disabled=!this.files.length">
<span id=sf style="margin:0 8px;font-size:.9em;font-weight:500;color:#333"></span>
<button id=ub class=bp onclick=ul() disabled>Upload</button><span id=st></span></div>
<div class=c><b>New folder</b>
<input type=text id=fn placeholder="folder name"><button class=bp onclick=mf()>Create</button><span id=ms></span></div>
</div>
<script>
var cwd='/';
function sz(b){return b>1048576?(b/1048576).toFixed(1)+' MB':b>1024?(b/1024).toFixed(1)+' KB':b+' B'}
function dt(ts){if(!ts||ts<946684800)return'<span class=g>—</span>';var d=new Date(ts*1000);return('0'+d.getDate()).slice(-2)+'/'+('0'+(d.getMonth()+1)).slice(-2)+'/'+d.getFullYear()+' '+('0'+d.getHours()).slice(-2)+':'+('0'+d.getMinutes()).slice(-2)}
function jp(n){return(cwd==='/'?'':cwd)+'/'+n}
function nav(p){cwd=p;document.getElementById('st').textContent='';document.getElementById('ms').textContent='';load()}
function par(){return cwd.lastIndexOf('/')<=0?'/':cwd.slice(0,cwd.lastIndexOf('/'))}
function loadStats(){
  fetch('/files/api/stats').then(function(r){return r.json();}).then(function(s){
    var pct=s.total>0?Math.round(s.used/s.total*100):0;
    var bar='<span style="display:inline-block;width:48px;height:5px;background:#e0e0e0;border-radius:3px"><span style="display:block;height:100%;background:#1565c0;border-radius:3px;width:'+pct+'%"></span></span>';
    document.getElementById('sp').innerHTML=sz(s.used)+' / '+sz(s.total)+' '+bar+' '+pct+'%';
  });
}
function load(){
  loadStats();
  var bc='<span style="color:#999;margin-right:6px">Location:</span><a onclick="nav(\'/\')">root</a>';
  if(cwd!=='/')cwd.split('/').filter(Boolean).forEach(function(s,i,a){bc+=' / <a onclick="nav(\'/'+a.slice(0,i+1).join('/')+'\')">'+s+'</a>';});
  document.getElementById('bl').innerHTML=bc;
  fetch('/files/api/list?path='+encodeURIComponent(cwd)).then(function(r){
    if(!r.ok)throw new Error('HTTP '+r.status);return r.json();
  }).then(function(a){
    var r=cwd!=='/'?'<tr class=er><td colspan=4><a onclick="nav(\''+par()+'\')">&#8593; ..</a></td></tr>':'';
    r+=a.length?a.map(function(f){var p=jp(f.name);return f.isDir?
      '<tr class="er dir"><td><a onclick="nav(\''+p+'\')">[&nbsp;'+f.name+'&nbsp;]</a></td><td class=g>—</td><td class=g>—</td><td><button class=bd onclick="rm(\''+p+'\')">&#x2715;</button></td></tr>':
      '<tr class=er><td>'+f.name+'</td><td>'+dt(f.lastWrite)+'</td><td class=g>'+sz(f.size)+'</td><td><button class=bs onclick="dl(\''+p+'\')">&#8595;</button><button class=bd onclick="rm(\''+p+'\')">&#x2715;</button></td></tr>';
    }).join(''):'<tr><td colspan=4 style="text-align:center;padding:24px;color:#ccc">Empty</td></tr>';
    document.getElementById('tb').innerHTML=r;
  }).catch(function(e){document.getElementById('tb').innerHTML='<tr><td colspan=4 style="color:#e53935;padding:12px">Error: '+e+'</td></tr>';});
}
function dl(p){location='/files/api/download?path='+encodeURIComponent(p)}
function rm(p){if(!confirm('Delete '+p+'?'))return;fetch('/files/api/delete?path='+encodeURIComponent(p),{method:'DELETE'}).then(load)}
function ul(){
  var f=document.getElementById('fi').files[0],st=document.getElementById('st');
  var d=new FormData();d.append('file',f);st.textContent='Uploading...';
  fetch('/files/api/upload?path='+encodeURIComponent(cwd),{method:'POST',body:d}).then(function(r){
    st.textContent=r.ok?'OK':'Error '+r.status;
    document.getElementById('fi').value='';document.getElementById('sf').textContent='';document.getElementById('ub').disabled=true;load();
  }).catch(function(e){st.textContent='Errore: '+e;});
}
function mf(){
  var n=document.getElementById('fn').value.trim(),ms=document.getElementById('ms');
  if(!n){ms.textContent='Enter a name';return;}
  fetch('/files/api/mkdir?path='+encodeURIComponent(jp(n)),{method:'POST'}).then(function(r){
    ms.textContent=r.ok?'OK':'Error '+r.status;
    document.getElementById('fn').value='';load();
  });
}
load();
</script></body></html>
)rawhtml";

WebFileManager& WebFileManager::GetInstance () {
    static WebFileManager Instance;
    return Instance;
}
WebFileManager& FileManager = WebFileManager::GetInstance();

WebFileManager::WebFileManager () {
    LOG(INFO, "WebFileManager", "Istanza creata");
}

// --- Configurazione ---

void WebFileManager::SetBasePath (const String& BasePath) {
    _BasePath = BasePath;
}

// --- Controllo runtime ---

void WebFileManager::Begin (AsyncWebServer* Server) {
    if (_IsStarted) return;
    if (Server == nullptr) {
        LOG(ERROR, "WebFileManager", "Server nullptr — Begin() ignorato");
        return;
    }
    _RegisterRoutes(Server);
    _IsStarted = true;
    LOG(INFO, "WebFileManager", "Avviato su /files/");
}

bool WebFileManager::IsStarted () {
    return _IsStarted;
}

// --- Internals ---

void WebFileManager::_RegisterRoutes (AsyncWebServer* Server) {
    Server->on("/ping", HTTP_GET, [](AsyncWebServerRequest* Request) {
        Request->send(200, "text/plain", "ok");
    });

    Server->on("/files/api/upload", HTTP_POST,
        [](AsyncWebServerRequest* Request) {
            Request->send(200);
        },
        [](AsyncWebServerRequest* Request, const String& Filename, size_t Index, uint8_t* Data, size_t Len, bool Final) {
            static File UploadFile;
            if (Index == 0) {
                String Dir  = Request->hasParam("path") ? Request->getParam("path")->value() : "/";
                String Path = (Dir == "/" ? "" : Dir) + "/" + Filename;
                UploadFile = FileSystem.OpenFile(Path, "w");
            }
            if (UploadFile) UploadFile.write(Data, Len);
            if (Final && UploadFile) {
                UploadFile.close();
                LOG(INFO, "WebFileManager", "Upload: " + Filename + " (" + String(Index + Len) + " bytes)");
            }
        }
    );

    Server->on("/files/api/list", HTTP_GET, [](AsyncWebServerRequest* Request) {
        String Path = Request->hasParam("path") ? Request->getParam("path")->value() : "/";
        std::vector<FileInfo> Files = FileSystem.ListFiles(Path);
        String Json = "[";
        for (size_t I = 0; I < Files.size(); I++) {
            if (I > 0) Json += ",";
            Json += "{\"name\":\"" + Files[I].Name + "\",\"size\":" + String(Files[I].Size) +
                    ",\"isDir\":" + (Files[I].IsDir ? "true" : "false") +
                    ",\"lastWrite\":" + String((unsigned long)Files[I].LastWrite) + "}";
        }
        Json += "]";
        Request->send(200, "application/json", Json);
    });

    Server->on("/files/api/download", HTTP_GET, [](AsyncWebServerRequest* Request) {
        if (!Request->hasParam("path")) { Request->send(400); return; }
        String Path = Request->getParam("path")->value();
        if (!FileSystem.FileExists(Path)) {
            LOG(WARNING, "WebFileManager", "Download: file non trovato: " + Path);
            Request->send(404);
            return;
        }
        LOG(INFO, "WebFileManager", "Download: " + Path);
        String Filename = Path.substring(Path.lastIndexOf('/') + 1);
        AsyncWebServerResponse* Response = Request->beginResponse(LittleFS, Path, "application/octet-stream");
        Response->addHeader("Content-Disposition", "attachment; filename=\"" + Filename + "\"");
        Request->send(Response);
    });

    Server->on("/files/api/delete", HTTP_DELETE, [](AsyncWebServerRequest* Request) {
        if (!Request->hasParam("path")) { Request->send(400); return; }
        String Path = Request->getParam("path")->value();
        bool Ok = FileSystem.IsDirectory(Path) ? FileSystem.DeleteDir(Path) : FileSystem.DeleteFile(Path);
        if (Ok) LOG(INFO,    "WebFileManager", "Delete: " + Path);
        else    LOG(ERROR,   "WebFileManager", "Delete fallito: " + Path);
        Request->send(Ok ? 200 : 500);
    });

    Server->on("/files/api/mkdir", HTTP_POST, [](AsyncWebServerRequest* Request) {
        if (!Request->hasParam("path")) { Request->send(400); return; }
        String Path = Request->getParam("path")->value();
        bool Ok = FileSystem.CreateDir(Path);
        if (Ok) LOG(INFO,  "WebFileManager", "Mkdir: " + Path);
        else    LOG(ERROR, "WebFileManager", "Mkdir fallito: " + Path);
        Request->send(Ok ? 200 : 500);
    });

    Server->on("/files/api/stats", HTTP_GET, [](AsyncWebServerRequest* Request) {
        String Json = "{\"total\":" + String(FileSystem.TotalBytes()) +
                      ",\"used\":"  + String(FileSystem.UsedBytes())  + "}";
        Request->send(200, "application/json", Json);
    });

    // HTML handler registrato per ultimo: fa prefix matching su /files/* quindi deve stare dopo le API
    auto HtmlHandler = [](AsyncWebServerRequest* Request) {
        Request->send(200, "text/html", _HtmlPage);
    };
    Server->on("/files",  HTTP_GET, HtmlHandler);
    Server->on("/files/", HTTP_GET, HtmlHandler);
}
