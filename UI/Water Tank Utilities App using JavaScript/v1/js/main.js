function setTankLevel(tankId, level) {
    // Find the progress bar for the specified tank
    var progressBar = document.getElementById(`progress-level-tank-${tankId}`);
    
    // Set the value of the progress bar
    progressBar.value = level;
    
    // Optionally, adjust the tank image or other visual elements here
    // For example, changing the water level image height based on the level
}

// Example usage:
setTankLevel('a', 50); // Sets Tank A's level to 50%
setTankLevel('b', 75); // Sets Tank B's level to 75%
setTankLevel('c', 25); // Sets Tank C's level to 25%
