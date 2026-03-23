console.log("=== External Script Execution Started ===");

var target = document.getElementById("js-target");
if (target) {
    console.log("Found #js-target, updating content...");
    target.innerHTML = "JS Execution SUCCESS! (Modified via app.js)";
    
    target.addEventListener("click", function() {
        console.log("Box clicked! Changing color to Red...");
        // In a real browser we'd change style, but here we'll just log
        // and maybe modify innerHTML to show interaction
        target.innerHTML = "<span>Clicked & Red!</span>";
    });
} else {
    console.log("Error: #js-target not found!");
}

console.log("=== External Script Execution Finished ===");
