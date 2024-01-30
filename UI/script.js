// Animate water levels for a group of tanks based on a percentage
function animateWaterLevel(group, levelPercentage) {
    // Find the tank group container based on the group identifier (A, B, or C)
    const tankGroup = document.querySelector('#group' + group + ' .tanks-row');
    
    // Calculate the new height for the water
    const tankHeight = 200; // Assuming the tank is 200px tall
    const newHeight = (levelPercentage / 100) * tankHeight;
    
    // Select all water elements within the group and animate the height
    const waterElements = tankGroup.querySelectorAll('.water');
    waterElements.forEach(water => {
        water.style.height = `${newHeight}px`;
    });
}

// Example usage - animate each group to 50% water level
animateWaterLevel('A', 25);
animateWaterLevel('B', 50);
animateWaterLevel('C', 100);

// You would call animateWaterLevel with the actual level from your data source
