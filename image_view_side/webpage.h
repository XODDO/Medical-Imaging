#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <Arduino.h>  // ensures types like PROGMEM are defined

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>CAVISCOPE Picture Manager</title>

<!-- Bootstrap 5 CSS -->
<link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/css/bootstrap.min.css" rel="stylesheet">
<link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">

<style>
  body { text-align:center; background-color:#f8f9fa; font-family: 'Segoe UI', sans-serif; }
  #main-image { max-width: 90%; max-height: 500px; margin: 20px auto; border: 2px solid #007bff; border-radius: 12px; }
  #gallery { display:flex; overflow-x:auto; margin-top:20px; padding:10px; background-color:#e9ecef; border-radius:10px; }
  .thumb { margin:0 5px; cursor:pointer; transition: transform 0.2s, border-color 0.2s; border: 2px solid #dee2e6; border-radius:6px; }
  .thumb:hover { transform: scale(1.1); border-color:#007bff; }
  .card { margin:5px; border-radius:10px; }
  .btn { min-width: 150px; }
</style>
</head>
<body>

<div class="container my-4">
  <h1 class="mb-4">CAVISCOPE PICTURE MANAGER</h1>

  <div class="alert alert-primary d-flex align-items-center justify-content-center">
    <i class="fa fa-exclamation-triangle me-2"></i> It takes 5-10 seconds to capture a photo.
  </div>

  <div class="d-flex justify-content-center flex-wrap mb-3 gap-2">
    <button class="btn btn-primary" onclick="rotatePhoto();"><i class="fa fa-undo me-1"></i> ROTATE PHOTO</button>
    <button class="btn btn-success" onclick="capturePhoto();"><i class="fa fa-camera me-1"></i> CAPTURE PHOTO</button>
    <button class="btn btn-info" onclick="location.reload();"><i class="fa fa-refresh me-1"></i> REFRESH PAGE</button>
    <button class="btn btn-warning" onclick="downloadImage();"><i class="fa fa-download me-1"></i> SAVE IMAGE</button>
  </div>

  <div class="text-center border rounded shadow-sm p-2 bg-white">
    <img src="saved-photo" id="photo" alt="Captured photo" class="img-fluid rounded" id="main-image">
  </div>

  <h3 class="mt-4 mb-2">Previous Images</h3>
  <div id="gallery" class="d-flex flex-row overflow-auto">
    <div class="card" style="min-width:120px;"><img src="..." class="img-fluid thumb" alt="recent image"></div>
    <div class="card" style="min-width:120px;"><img src="..." class="img-fluid thumb" alt="previous image 2"></div>
    <div class="card" style="min-width:120px;"><img src="..." class="img-fluid thumb" alt="previous image 3"></div>
    <div class="card" style="min-width:120px;"><img src="..." class="img-fluid thumb" alt="previous image 4"></div>
  </div>
</div>

<script>
  let deg = 0;

  function capturePhoto() {
    fetch("/capture")
      .then(() => console.log("Photo capture requested"))
      .catch(err => console.error(err));
  }

  function rotatePhoto() {
    deg += 90;
    const img = document.getElementById("photo");
    img.style.transform = "rotate(" + deg + "deg)";
  }

  function showMain(elem) {
    const main = document.getElementById("photo");
    main.src = elem.src;
    deg = 0;
    main.style.transform = "rotate(0deg)";
  }

  function downloadImage() {
    const img = document.getElementById("photo");
    const link = document.createElement('a');
    link.href = img.src;
    link.download = 'caviscope_photo.jpg';
    link.click();
  }
</script>

</body>
</html>
)rawliteral";

#endif
