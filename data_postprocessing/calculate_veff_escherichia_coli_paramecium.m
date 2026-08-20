function calculate_veff_escherichia_coli_paramecium(rootPath)
%CALCULATE_VEFF_ESCHERICHIA_COLI_PARAMECIUM Calculate effective velocity.
%   CALCULATE_VEFF_ESCHERICHIA_COLI_PARAMECIUM(ROOT_PATH) processes CSV
%   pairs for Escherichia coli or Paramecium in ROOT_PATH and its immediate
%   child directories. The current directory is used when ROOT_PATH is omitted.

if nargin < 1 || isempty(rootPath)
    rootPath = pwd;
end

if ~(ischar(rootPath) || (isstring(rootPath) && isscalar(rootPath)))
    error('rootPath must be a character vector or a string scalar.');
end

rootPath = char(rootPath);
if ~isfolder(rootPath)
    error('Data root does not exist: %s', rootPath);
end

smoothWindow = 5;
minSegmentLength = 5;
minStraightPoints = 3;

straightAngleDeg = 20;
speedPercentile = 60;
maxTransSpeed = 30;

straightAngleRad = straightAngleDeg * pi / 180;

targetFolders = struct('name', {}, 'path', {});

[~, rootName] = fileparts(rootPath);
targetFolders(1).name = rootName;
targetFolders(1).path = rootPath;

folderInfo = dir(rootPath);
folderInfo = folderInfo([folderInfo.isdir]);
folderInfo = folderInfo(~ismember({folderInfo.name}, {'.', '..'}));

for i = 1:length(folderInfo)
    targetFolders(end+1).name = folderInfo(i).name;
    targetFolders(end).path = fullfile(rootPath, folderInfo(i).name);
end

allRows = {};
allSkipRows = {};
allSummaryRows = {};

for f = 1:length(targetFolders)

    folderName = targetFolders(f).name;
    folderPath = targetFolders(f).path;

    csvFiles = dir(fullfile(folderPath, '*.csv'));

    if isempty(csvFiles)
        continue;
    end

    fileNames = string({csvFiles.name});

    resultMask = startsWith(fileNames, "Veff_") | startsWith(fileNames, "All_Folders_");
    csvFiles = csvFiles(~resultMask);
    fileNames = string({csvFiles.name});

    circleMask = contains(lower(fileNames), "circle");
    kinMask = contains(lower(fileNames), "kinematics");

    circleFiles = csvFiles(circleMask);
    kinFiles = csvFiles(kinMask);

    if isempty(circleFiles) || isempty(kinFiles)
        continue;
    end

    fprintf('\nProcessing folder: %s\n', folderName);

    folderRows = {};
    folderSkipRows = {};
    folderVeff = [];

    totalCircleRowCount = 0;
    validVeffCount = 0;
    skippedRowCount = 0;

    for cf = 1:length(circleFiles)

        circleName = circleFiles(cf).name;
        circlePath = fullfile(folderPath, circleName);

        prefix = getFilePrefix(circleName);

        if strlength(prefix) == 0
            skippedRowCount = skippedRowCount + 1;
            reason = "Failed to identify the circle-file prefix";
            folderSkipRows = [folderSkipRows; {folderName, circleName, "", NaN, NaN, NaN, NaN, reason}];
            continue;
        end

        kinNames = string({kinFiles.name});
        matchID = find(startsWith(kinNames, prefix) & contains(lower(kinNames), "kinematics"), 1);

        if isempty(matchID)
            skippedRowCount = skippedRowCount + 1;
            reason = "No matching kinematics file was found";
            folderSkipRows = [folderSkipRows; {folderName, circleName, "", NaN, NaN, NaN, NaN, reason}];
            continue;
        end

        kinName = kinFiles(matchID).name;
        kinPath = fullfile(folderPath, kinName);

        fprintf('  Matched files: %s  -->  %s\n', circleName, kinName);

        try
            C = readtable(circlePath);
        catch
            warning(['Unable to read circle file: ', circlePath]);
            skippedRowCount = skippedRowCount + 1;
            reason = "Failed to read circle file";
            folderSkipRows = [folderSkipRows; {folderName, circleName, kinName, NaN, NaN, NaN, NaN, reason}];
            continue;
        end

        try
            K = readtable(kinPath);
        catch
            warning(['Unable to read kinematics file: ', kinPath]);
            skippedRowCount = skippedRowCount + 1;
            reason = "Failed to read kinematics file";
            folderSkipRows = [folderSkipRows; {folderName, circleName, kinName, NaN, NaN, NaN, NaN, reason}];
            continue;
        end

        if width(C) < 15
            skippedRowCount = skippedRowCount + height(C);
            reason = "Circle file contains fewer than 15 columns";
            folderSkipRows = [folderSkipRows; {folderName, circleName, kinName, NaN, NaN, NaN, NaN, reason}];
            continue;
        end

        if width(K) < 9
            skippedRowCount = skippedRowCount + height(C);
            reason = "Kinematics file contains fewer than 9 columns";
            folderSkipRows = [folderSkipRows; {folderName, circleName, kinName, NaN, NaN, NaN, NaN, reason}];
            continue;
        end

        c_video_id = getNumericColumn(C, 1);
        c_idx = getNumericColumn(C, 2);
        c_enter_frame = getNumericColumn(C, 3);
        c_exit_frame = getNumericColumn(C, 4);
        c_enter_time = getNumericColumn(C, 5);
        c_exit_time = getNumericColumn(C, 6);

        c_enter_x = getNumericColumn(C, 7);
        c_enter_y = getNumericColumn(C, 8);
        c_exit_x = getNumericColumn(C, 9);
        c_exit_y = getNumericColumn(C, 10);

        c_inside_duration = getNumericColumn(C, 13);
        c_inside_distance = getNumericColumn(C, 15);

        k_video_id = getNumericColumn(K, 1);
        k_idx = getNumericColumn(K, 2);
        k_frame = getNumericColumn(K, 3);
        k_time = getNumericColumn(K, 4);
        k_angle = getNumericColumn(K, 7);
        k_speed = getNumericColumn(K, 9);

        for r = 1:height(C)

            totalCircleRowCount = totalCircleRowCount + 1;

            videoID = c_video_id(r);
            thisIdx = c_idx(r);

            enterFrame = c_enter_frame(r);
            exitFrame = c_exit_frame(r);

            enterTime = c_enter_time(r);
            exitTime = c_exit_time(r);

            enterX = c_enter_x(r);
            enterY = c_enter_y(r);
            exitX = c_exit_x(r);
            exitY = c_exit_y(r);

            insideDuration = c_inside_duration(r);
            insideDistance = c_inside_distance(r);

            if ~isfinite(thisIdx) || ~isfinite(enterFrame) || ~isfinite(exitFrame)
                skippedRowCount = skippedRowCount + 1;
                reason = "Invalid idx or frame value in circle row";
                folderSkipRows = [folderSkipRows; {folderName, circleName, kinName, r, videoID, thisIdx, enterFrame, reason}];
                continue;
            end

            if exitFrame < enterFrame
                temp = enterFrame;
                enterFrame = exitFrame;
                exitFrame = temp;
            end

            if ~isfinite(insideDuration) || insideDuration <= 0
                insideDuration = exitTime - enterTime;
            end

            if ~isfinite(insideDuration) || insideDuration <= 0
                skippedRowCount = skippedRowCount + 1;
                reason = "Invalid residence time";
                folderSkipRows = [folderSkipRows; {folderName, circleName, kinName, r, videoID, thisIdx, enterFrame, reason}];
                continue;
            end

            if ~isfinite(insideDistance) || insideDistance <= 0
                skippedRowCount = skippedRowCount + 1;
                reason = "Invalid travel distance";
                folderSkipRows = [folderSkipRows; {folderName, circleName, kinName, r, videoID, thisIdx, enterFrame, reason}];
                continue;
            end

            segMask = k_idx == thisIdx & ...
                      k_frame >= enterFrame & ...
                      k_frame <= exitFrame & ...
                      isfinite(k_frame) & ...
                      isfinite(k_angle) & ...
                      isfinite(k_speed) & ...
                      k_speed >= 0;

            if isfinite(videoID) && any(isfinite(k_video_id))
                segMask = segMask & k_video_id == videoID;
            end

            segFrame = k_frame(segMask);
            segTime = k_time(segMask);
            segAngle = k_angle(segMask);
            segSpeed = k_speed(segMask);

            if length(segSpeed) < minSegmentLength
                skippedRowCount = skippedRowCount + 1;
                reason = "Insufficient points in the matching kinematics segment";
                folderSkipRows = [folderSkipRows; {folderName, circleName, kinName, r, videoID, thisIdx, enterFrame, reason}];
                continue;
            end

            [segFrame, order] = sort(segFrame);
            segTime = segTime(order);
            segAngle = segAngle(order);
            segSpeed = segSpeed(order);

            [segFrame, uniqueID] = unique(segFrame, 'stable');
            segTime = segTime(uniqueID);
            segAngle = segAngle(uniqueID);
            segSpeed = segSpeed(uniqueID);

            if length(segSpeed) < minSegmentLength
                skippedRowCount = skippedRowCount + 1;
                reason = "Insufficient points in the trajectory segment after duplicate removal";
                folderSkipRows = [folderSkipRows; {folderName, circleName, kinName, r, videoID, thisIdx, enterFrame, reason}];
                continue;
            end

            [transSpeed, nForwardPoints, segMeanSpeed, speedThreshold, angleThresholdDeg, method] = ...
                calcForwardTransSpeed(segSpeed, segAngle, ...
                smoothWindow, straightAngleRad, speedPercentile, minStraightPoints);

            if ~isfinite(transSpeed) || transSpeed <= 0
                skippedRowCount = skippedRowCount + 1;
                reason = "No valid forward-scattering straight segment was identified";
                folderSkipRows = [folderSkipRows; {folderName, circleName, kinName, r, videoID, thisIdx, enterFrame, reason}];
                continue;
            end

            if transSpeed > maxTransSpeed
                skippedRowCount = skippedRowCount + 1;
                reason = "Translational speed exceeds 30";
                folderSkipRows = [folderSkipRows; {folderName, circleName, kinName, r, videoID, thisIdx, enterFrame, reason}];
                continue;
            end

            Veff = insideDistance / insideDuration / transSpeed;

            validVeffCount = validVeffCount + 1;
            folderVeff = [folderVeff; Veff];

            rowData = { ...
                folderName, ...
                circleName, ...
                kinName, ...
                r, ...
                videoID, ...
                thisIdx, ...
                enterFrame, ...
                exitFrame, ...
                enterTime, ...
                exitTime, ...
                enterX, ...
                enterY, ...
                exitX, ...
                exitY, ...
                insideDuration, ...
                insideDistance, ...
                segFrame(1), ...
                segFrame(end), ...
                length(segSpeed), ...
                segMeanSpeed, ...
                speedThreshold, ...
                transSpeed, ...
                nForwardPoints, ...
                angleThresholdDeg, ...
                Veff, ...
                string(method)};

            folderRows = [folderRows; rowData];
            allRows = [allRows; rowData];

        end
    end

    folderResult = safeCell2Table(folderRows, ...
        'VariableNames', { ...
        'Folder', ...
        'Circle_File', ...
        'Kinematics_File', ...
        'Circle_Row', ...
        'Video_ID', ...
        'Idx', ...
        'Enter_Frame', ...
        'Exit_Frame', ...
        'Enter_Time', ...
        'Exit_Time', ...
        'Enter_X', ...
        'Enter_Y', ...
        'Exit_X', ...
        'Exit_Y', ...
        'Inside_Duration_M', ...
        'Inside_Distance_O', ...
        'Matched_Frame_Start', ...
        'Matched_Frame_End', ...
        'Segment_Point_Number', ...
        'Segment_Mean_Speed', ...
        'Speed_Threshold', ...
        'Translational_Speed', ...
        'Forward_Point_Number', ...
        'Angle_Threshold_Deg', ...
        'Veff', ...
        'Method'});

    folderSkipResult = safeCell2Table(folderSkipRows, ...
        'VariableNames', { ...
        'Folder', ...
        'Circle_File', ...
        'Kinematics_File', ...
        'Circle_Row', ...
        'Video_ID', ...
        'Idx', ...
        'Enter_Frame', ...
        'Skip_Reason'});

    meanVeff = meanFinite(folderVeff);
    stdVeff = stdFinite(folderVeff);

    folderSummary = table( ...
        string(folderName), ...
        totalCircleRowCount, ...
        validVeffCount, ...
        skippedRowCount, ...
        meanVeff, ...
        stdVeff, ...
        'VariableNames', { ...
        'Folder', ...
        'Total_Circle_Row_Number', ...
        'Valid_Veff_Number', ...
        'Skipped_Row_Number', ...
        'Mean_Veff', ...
        'STD_Veff'});

    allSummaryRows = [allSummaryRows; { ...
        folderName, ...
        totalCircleRowCount, ...
        validVeffCount, ...
        skippedRowCount, ...
        meanVeff, ...
        stdVeff}];

    allSkipRows = [allSkipRows; folderSkipRows];

    if height(folderResult) > 0
        writetable(folderResult, fullfile(folderPath, 'Veff_All_Rows_In_This_Folder.csv'));
    end

    writetable(folderSummary, fullfile(folderPath, 'Veff_Folder_Summary.csv'));

    if height(folderSkipResult) > 0
        writetable(folderSkipResult, fullfile(folderPath, 'Veff_Skipped_Rows.csv'));
    end

    fprintf('  Folder %s completed: valid Veff = %d, skipped = %d, mean Veff = %.6f, SD = %.6f\n', ...
        folderName, validVeffCount, skippedRowCount, meanVeff, stdVeff);

end

if isempty(allRows)
    warning('No Veff values were successfully calculated.');
    return;
end

allResult = safeCell2Table(allRows, ...
    'VariableNames', { ...
    'Folder', ...
    'Circle_File', ...
    'Kinematics_File', ...
    'Circle_Row', ...
    'Video_ID', ...
    'Idx', ...
    'Enter_Frame', ...
    'Exit_Frame', ...
    'Enter_Time', ...
    'Exit_Time', ...
    'Enter_X', ...
    'Enter_Y', ...
    'Exit_X', ...
    'Exit_Y', ...
    'Inside_Duration_M', ...
    'Inside_Distance_O', ...
    'Matched_Frame_Start', ...
    'Matched_Frame_End', ...
    'Segment_Point_Number', ...
    'Segment_Mean_Speed', ...
    'Speed_Threshold', ...
    'Translational_Speed', ...
    'Forward_Point_Number', ...
    'Angle_Threshold_Deg', ...
    'Veff', ...
    'Method'});

summaryResult = safeCell2Table(allSummaryRows, ...
    'VariableNames', { ...
    'Folder', ...
    'Total_Circle_Row_Number', ...
    'Valid_Veff_Number', ...
    'Skipped_Row_Number', ...
    'Mean_Veff', ...
    'STD_Veff'});

skipResult = safeCell2Table(allSkipRows, ...
    'VariableNames', { ...
    'Folder', ...
    'Circle_File', ...
    'Kinematics_File', ...
    'Circle_Row', ...
    'Video_ID', ...
    'Idx', ...
    'Enter_Frame', ...
    'Skip_Reason'});

writetable(allResult, fullfile(rootPath, 'All_Folders_All_Veff.csv'));
writetable(summaryResult, fullfile(rootPath, 'All_Folders_Veff_Summary.csv'));

if height(skipResult) > 0
    writetable(skipResult, fullfile(rootPath, 'All_Folders_Skipped_Rows.csv'));
end

outXlsx = fullfile(rootPath, 'All_Folders_Veff_Result.xlsx');

writetable(allResult, outXlsx, 'Sheet', 'All_Veff');
writetable(summaryResult, outXlsx, 'Sheet', 'Summary_By_Folder');

if height(skipResult) > 0
    writetable(skipResult, outXlsx, 'Sheet', 'Skipped_Rows');
end

disp('All calculations have been completed.');
disp('Outputs in the current root directory:');
disp(fullfile(rootPath, 'All_Folders_All_Veff.csv'));
disp(fullfile(rootPath, 'All_Folders_Veff_Summary.csv'));
disp(fullfile(rootPath, 'All_Folders_Veff_Result.xlsx'));

end

function [transSpeed, nForwardPoints, segMeanSpeed, speedThreshold, angleThresholdDeg, method] = ...
    calcForwardTransSpeed(speed, angle, smoothWindow, straightAngleRad, speedPercentile, minStraightPoints)

    speed = speed(:);
    angle = angle(:);

    valid = isfinite(speed) & isfinite(angle) & speed >= 0;

    speed = speed(valid);
    angle = angle(valid);

    transSpeed = NaN;
    nForwardPoints = 0;
    segMeanSpeed = NaN;
    speedThreshold = NaN;
    angleThresholdDeg = straightAngleRad * 180 / pi;
    method = "no_forward_segment";

    if length(speed) < minStraightPoints
        method = "too_few_points";
        return;
    end

    win = min(smoothWindow, length(speed));

    smoothSpeed = movmean(speed, win);

    dtheta = [0; abs(atan2(sin(diff(angle)), cos(diff(angle))))];
    smoothDtheta = movmean(dtheta, win);

    valid2 = isfinite(smoothSpeed) & isfinite(smoothDtheta);

    smoothSpeed = smoothSpeed(valid2);
    smoothDtheta = smoothDtheta(valid2);

    if length(smoothSpeed) < minStraightPoints
        method = "too_few_points_after_smooth";
        return;
    end

    segMeanSpeed = meanFinite(smoothSpeed);

    speedP = percentileNoToolbox(smoothSpeed, speedPercentile);
    speedThreshold = max(segMeanSpeed, speedP);

    speedMask = smoothSpeed >= speedThreshold;
    angleMask = smoothDtheta <= straightAngleRad;

    forwardMask = speedMask & angleMask;
    forwardMask = keepLongContinuousSegments(forwardMask, minStraightPoints);

    forwardSpeed = smoothSpeed(forwardMask);

    if length(forwardSpeed) >= minStraightPoints
        candidateSpeed = meanFinite(forwardSpeed);

        if candidateSpeed >= segMeanSpeed
            transSpeed = candidateSpeed;
            nForwardPoints = length(forwardSpeed);
            method = "speed_ge_max_mean_p60_and_dtheta_small";
            return;
        end
    end

    speedThreshold = segMeanSpeed;

    forwardMask = smoothSpeed >= speedThreshold & angleMask;
    forwardMask = keepLongContinuousSegments(forwardMask, minStraightPoints);

    forwardSpeed = smoothSpeed(forwardMask);

    if length(forwardSpeed) >= minStraightPoints
        candidateSpeed = meanFinite(forwardSpeed);

        if candidateSpeed >= segMeanSpeed
            transSpeed = candidateSpeed;
            nForwardPoints = length(forwardSpeed);
            method = "speed_ge_mean_and_dtheta_small";
            return;
        end
    end

    straightSpeed = smoothSpeed(angleMask);

    if length(straightSpeed) >= minStraightPoints

        p50 = percentileNoToolbox(straightSpeed, 50);
        forwardSpeed = straightSpeed(straightSpeed >= p50);

        if length(forwardSpeed) >= minStraightPoints
            candidateSpeed = meanFinite(forwardSpeed);

            if candidateSpeed >= segMeanSpeed
                transSpeed = candidateSpeed;
                nForwardPoints = length(forwardSpeed);
                speedThreshold = p50;
                method = "top_half_speed_among_straight_points";
                return;
            end
        end
    end

end

function x = getNumericColumn(T, colIndex)

    x = T{:, colIndex};

    if iscell(x)
        x = str2double(x);
    elseif isstring(x)
        x = str2double(x);
    elseif ischar(x)
        x = str2double(cellstr(x));
    else
        x = double(x);
    end

end

function q = percentileNoToolbox(x, p)

    x = x(:);
    x = x(isfinite(x));

    if isempty(x)
        q = NaN;
        return;
    end

    x = sort(x);
    n = length(x);

    if n == 1
        q = x;
        return;
    end

    pos = 1 + (p / 100) * (n - 1);

    lower = floor(pos);
    upper = ceil(pos);

    if lower == upper
        q = x(lower);
    else
        q = x(lower) + (pos - lower) * (x(upper) - x(lower));
    end

end

function maskOut = keepLongContinuousSegments(maskIn, minLen)

    maskIn = maskIn(:);
    maskOut = false(size(maskIn));

    if isempty(maskIn)
        return;
    end

    d = diff([false; maskIn; false]);

    startIdx = find(d == 1);
    endIdx = find(d == -1) - 1;

    for i = 1:length(startIdx)

        segLen = endIdx(i) - startIdx(i) + 1;

        if segLen >= minLen
            maskOut(startIdx(i):endIdx(i)) = true;
        end

    end

end

function prefix = getFilePrefix(fileName)

    token = regexp(char(fileName), '^\d+', 'match', 'once');

    if isempty(token)
        prefix = "";
    else
        prefix = string(token);
    end

end

function m = meanFinite(x)

    x = x(:);
    x = x(isfinite(x));

    if isempty(x)
        m = NaN;
    else
        m = mean(x);
    end

end

function s = stdFinite(x)

    x = x(:);
    x = x(isfinite(x));

    if isempty(x)
        s = NaN;
    elseif length(x) == 1
        s = 0;
    else
        s = std(x);
    end

end

function T = safeCell2Table(rows, varargin)

    variableNamesIndex = [];
    for i = 1:2:length(varargin)
        if ischar(varargin{i}) && strcmpi(varargin{i}, 'VariableNames')
            variableNamesIndex = i;
            break;
        end
    end

    if isempty(variableNamesIndex) || variableNamesIndex == length(varargin)
        error('safeCell2Table requires a VariableNames argument.');
    end

    variableNames = varargin{variableNamesIndex + 1};
    if isempty(rows)
        rows = cell(0, length(variableNames));
    end

    if size(rows, 2) ~= length(variableNames)
        error('safeCell2Table: rows has %d columns, but VariableNames has %d elements.', ...
            size(rows, 2), length(variableNames));
    end

    T = cell2table(rows, varargin{:});

end
