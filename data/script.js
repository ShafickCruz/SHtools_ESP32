function showInfo() {
  var modal = document.getElementById("infoModal");
  modal.style.display = "block";
}

document.addEventListener("DOMContentLoaded", (event) => {
  var modal = document.getElementById("infoModal");
  var span = document.getElementsByClassName("close")[0];

  span.onclick = function () {
    modal.style.display = "none";
  };

  window.onclick = function (event) {
    if (event.target == modal) {
      modal.style.display = "none";
    }
  };
});
