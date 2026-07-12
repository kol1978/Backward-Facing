%% This code serves as a format to read airfoilLES files from database
% Authors: Het D. Patel, North Carolina State University 
%          Chi-An Yeh, North Carolina State University 
%          Kunihiko Taira, University of California, Los Angeles

%% - Setup -------------------------------------------------------------- %

% Clear workspace
clear; clc; close all;

% Root directory where data is stored
DIR = './data';


%% -- Reading parameters ------------------------------------------------ %
% Use this section to read airfoilLES_parameters.h5 file from database
filename_parameters = fullfile(DIR, 'airfoilLES_parameters.h5'); 
Re = h5read(filename_parameters, '/Re');     % Reynolds number
dt = h5read(filename_parameters, '/dt');     % Time step between snapshots (convective time unit)


%% -- Reading grid information ------------------------------------------ %
% Use this section to read airfoilLES_grid.h5 file from database
filename_grid = fullfile(DIR, 'airfoilLES_grid.h5'); 
x  = h5read(filename_grid, '/x');           % x-coordinate for flow-field grid
y  = h5read(filename_grid, '/y');           % y-coordinate for flow-field grid
xa = h5read(filename_grid, '/xa');          % x-coordinate for airfoil grid
ya = h5read(filename_grid, '/ya');          % y-coordinate for airfoil grid
w  = h5read(filename_grid, '/w');           % cell volume for flow-field grid


%% -- Reading time-averaged mid-span flow information ------------------- %
% Use this section to read airfoilLES_mean_midspan.h5 file from database
filename_mean_midspan = fullfile(DIR, 'airfoilLES_mean_midspan.h5'); 
ux_mean = h5read(filename_mean_midspan, '/ux_mean');
uy_mean = h5read(filename_mean_midspan, '/uy_mean');
uz_mean = h5read(filename_mean_midspan, '/uz_mean');

% Plot the mean mid-span flow 
figure;
scatter(x, y, 10, ux_mean, 'filled') % can also use scattered interpolation
hold on;
fill(xa, ya, [0.6 0.6 0.6]);
axis equal; box on; hold off;
axis([-0.2, 2, -1, 1])
caxis([-0.2 1.2]), colorbar
xlabel('$x/L_c$','Interpreter','Latex');
ylabel('$y/L_c$','Interpreter','Latex');
title('Mean flow over mid-span slice');
    

%% ------------------- Reading time- and z-averaged flow information ---- %
% Use this section to read airfoilLES_mean_zavg.h5 file from database
filename_mean_zavg = fullfile(DIR, 'airfoilLES_mean_zavg.h5'); 
ux_zavg_mean = h5read(filename_mean_zavg, '/ux_zavg_mean');
uy_zavg_mean = h5read(filename_mean_zavg, '/uy_zavg_mean');
uz_zavg_mean = h5read(filename_mean_zavg, '/uz_zavg_mean');

% Plot the mean spanwise-averaged flow 
figure;
scatter(x, y, 10, ux_zavg_mean, 'filled') % can also use scattered interpolation
hold on;
fill(xa, ya, [0.6 0.6 0.6]);
axis equal; box on; hold off;
axis([-0.2, 2, -1, 1]);
caxis([-0.2 1.2]); colorbar;
xlabel('$x/L_c$','Interpreter','Latex');
ylabel('$y/L_c$','Interpreter','Latex');
title('Time- and spanwise-averaged flow');


%% ------------------- Reading flow-field snapshots---------------------- %
% Use this section to read airfoilLES_t#####.h5 files from database

% Specify list of snapshots to read
jt_list = 1:1:100;   % Note: jt_list = 1:16000 will return all snapshots

% Allocate storage for data
Q  = zeros(length(x)*3, length(jt_list));
jj = 0;

% Read mid-span snapshots
for jt = jt_list
    jj       = jj + 1;
    
    filename = fullfile(DIR,sprintf('airfoilLES_t%05u.h5', jt));
    ux       = h5read(filename, '/ux');
    uy       = h5read(filename, '/uy');
    uz       = h5read(filename, '/uz');
    
    % stacking velocities as a state vector (mean removed)
    Q(:, jj) = [ux - ux_mean; uy - uy_mean; uz - uz_mean];
end

% Read spanwise-averaged snapshots
for jt = jt_list
    filename = fullfile(DIR,sprintf('airfoilLES_t%05u.h5', jt));
    ux_zavg  = h5read(filename, '/ux_zavg');
    uy_zavg  = h5read(filename, '/uy_zavg');
    uz_zavg  = h5read(filename, '/uz_zavg');    
end

% Plot a snapshot of the mid-span slice 
figure ;
scatter(x, y, 10, ux, 'filled') % can also use scattered interpolation
hold on;
fill(xa, ya, [0.6 0.6 0.6]);
axis equal; box on; hold off;
axis([-0.2, 2, -1, 1]);
caxis([-0.2 1.2]); colorbar;
xlabel('$x/L_c$','Interpreter','Latex');
ylabel('$y/L_c$','Interpreter','Latex');
title('Instataneous flow over mid-span slice');


%% -- POD modes --------------------------------------------------------- %

% Stacking weights Remove mean
WeightVec = [w; w; w];

% Compute POD modes
numout = 10;
[phi,lambda,a] = calc_POD(Q, WeightVec, numout);

% Plot POD eigen spectra
figure; 
loglog(1:numout, lambda, 'ko-');
title('POD eigenvalues for mid-span velocity field', 'Interpreter','Latex');
xlabel('$k$','Interpreter','Latex');
ylabel('$\lambda$','Interpreter','Latex');

% Plot leading POD mode
figure;
scatter(x, y, 10, phi(1:length(x),1), 'filled'); % can also use scattered interpolation
hold on;
fill(xa, ya, [0.6 0.6 0.6]);
axis equal; box on; hold off;
axis([-0.2, 2, -1, 1]);
caxis([-0.2 1.2]); colorbar;
xlabel('$x/L_c$','Interpreter','Latex');
ylabel('$y/L_c$','Interpreter','Latex');
title('Leading POD mode of the $u_x$ component','Interpreter','Latex');


