using StaticArrays
using StaticArrays: dot

const global Vec3 = SVector{3,Float64}
const global Vec2 = SVector{2,Float64}

const global HEIGHT = 50
const global ASCII_RATIO = 2.1
const global WIDTH = round(Int, HEIGHT * ASCII_RATIO * 16 / 9)
const global BACKGROUND = ' '
const global SYMBOLS = raw".,-~:;=!*#$@"

const global DONUT_RADIUS = 1.7
const global DONUT_THICKNESS = 0.9
const global DONUT_DISTANCE = 5

const global K1 =
    WIDTH * DONUT_DISTANCE * 0.2 / (DONUT_RADIUS + DONUT_THICKNESS)
const global LIGHT_DIRECTION = Vec3(0, -1, 1)

const global SPEED_MULTIPLIER = 2
const global Y_SPEED = 0.5 * SPEED_MULTIPLIER
const global X_SPEED = 1 * SPEED_MULTIPLIER

function draw(buffer::Matrix{Char})::Nothing
    println("\033[1;1H") # move cursor to beginning
    outstring = join(map(x -> join(x), eachrow(buffer)), "\n")
    print(outstring)
end

function main()::Nothing
    Base.exit_on_sigint(false)

    print("\033[2J") # clear screen
    print("\e[?25l") # hide cursor

    try
        loop()
    catch k
        isa(k, InterruptException) ? println("\e[?25h") : throw(k)
    end

end


function loop()::Nothing

    angle_y = 0.0
    angle_x = 0.0
    previoustime = time_ns()

    while true
        # initialize buffers
        buffer = fill(BACKGROUND, HEIGHT, WIDTH)
        zbuffer = fill(1000.0, HEIGHT, WIDTH)

        # FPS
        starttime = time_ns()
        lastframetime = starttime - previoustime
        fps = round(Int, 1 / (lastframetime / 10^9))
        previoustime = starttime
        fpsstring = "FPS:$fps"
        buffer[end, end-Base.length(fpsstring)+1:end] = Vector{Char}(fpsstring)

        # precompute cosines and sines
        cosx = cos(angle_x)
        cosy = cos(angle_y)
        sinx = sin(angle_x)
        siny = sin(angle_y)

        # cross section along the shape of the donut
        for angle_cross_section in range(0, 2π, length = 1100)#, step = 0.02)
            # precompute cos and sin of the cross-secion angle along z 
            cosa = cos(angle_cross_section)
            sina = sin(angle_cross_section)

            # position of the center of the cross-section w.r.t donut center
            cc_center::Vec3 = Vec3(DONUT_RADIUS, 0, 0)

            # rotate center of the cross-section w.r.t. donut center
            cc_center = rotate_z(cc_center, cosa, sina)
            cc_center = rotate_x(cc_center, cosx, sinx)
            cc_center = rotate_y(cc_center, cosy, siny)

            # x along the circumference of the donut cross section
            for angle_point in range(0, 2π, length = 400)#, step = 0.07)

                # position of the point w.r.t. donut center
                p::Vec3 = Vec3(
                    DONUT_RADIUS + DONUT_THICKNESS * cos(angle_point),
                    0,
                    DONUT_THICKNESS * sin(angle_point),
                )
                p = rotate_z(p, cosa, sina)
                p = rotate_x(p, cosx, sinx)
                p = rotate_y(p, cosy, siny)

                # compute normal as the vector from the cc_center to the point
                normal::Vec3 = normalize(p - cc_center)

                # position w.r.t. camera
                p = Vec3(p.x, p.y, p.z + DONUT_DISTANCE)
                p.z <= 0 && continue

                # project in 2d
                projection2d::Vec2 = project(p)
                i, j = quantizeshift(projection2d)

                # if point is out of screen or covered by another point, skip
                i ∉ 0:WIDTH || j ∉ 0:HEIGHT || zbuffer[j, i] <= p.z && continue

                # compute luminance and assing a corresponding symbol
                luminance = dot(-normal, normalize(LIGHT_DIRECTION))
                luminance < 0 && (luminance = 0)
                index = floor(Int, luminance * Base.length(SYMBOLS)) + 1
                symbol::Char = SYMBOLS[index]

                # update buffers
                buffer[j, i] = symbol
                zbuffer[j, i] = p.z
            end
        end

        angle_y += Y_SPEED / fps
        angle_x += X_SPEED / fps

        draw(buffer)
    end
end

function rotate_zyx(p::SVector{3}, cosx, cosy, cosz, sinx, siny, sinz)::Vec3
    sinxsiny = sinx * siny
    cosxsiny = cosx * siny
    return Vec3(
        # x
        p.x * (cosy * cosz) - p.y * (cosy * sinz) + p.z * (siny),
        # y
        p.x * (cosx * sinz + sinxsiny * cosz) +
        p.y * (cosx * cosz - sinxsiny * sinz) - p.z * (sinx * cosy),
        # z
        p.x * (sinx * sinz - cosxsiny * cosz) +
        p.y * (sinx * cosz + cosxsiny * sinz) +
        p.z * (cosx * cosy),
    )
end


function rotate_z(point::SVector{3}, cosangle::Real, sinangle::Real)::Vec3
    return Vec3(
        cosangle * point.x - sinangle * point.y,
        sinangle * point.x - cosangle * point.y,
        point.z,
    )
end


function rotate_y(point::SVector{3}, cosangle::Real, sinangle::Real)::Vec3
    return Vec3(
        point.x * cosangle + point.z * sinangle,
        point.y,
        -point.x * sinangle + point.z * cosangle,
    )
end

function rotate_x(point::SVector{3}, cosangle::Real, sinangle::Real)::Vec3
    return Vec3(
        point.x,
        point.y * cosangle - point.z * sinangle,
        point.y * sinangle + point.z * cosangle,
    )
end


function rotate_z(point::SVector{3}, angle::Real)::Vec3
    z_rotation_matrix = SMatrix{3,3,Real}(
        cos(angle),
        sin(angle),
        0,
        -sin(angle),
        cos(angle),
        0,
        0,
        0,
        1,
    )
    return z_rotation_matrix * point
end

function rotate_y(point::SVector{3}, angle::Real)::Vec3
    y_rotation_matrix = SMatrix{3,3,Real}(
        cos(angle),
        0,
        -sin(angle),
        0,
        1,
        0,
        sin(angle),
        0,
        cos(angle),
    )
    return y_rotation_matrix * point
end

function rotate_x(point::SVector{3}, angle::Real)::Vec3
    x_rotation_matrix = SMatrix{3,3,Real}(
        1,
        0,
        0,
        0,
        cos(angle),
        sin(angle),
        0,
        -sin(angle),
        cos(angle),
    )
    return x_rotation_matrix * point
end

project(p::SVector{3})::Vec2 =
    Vec2(K1 * p.x / (DONUT_DISTANCE + p.z), K1 * p.y / (DONUT_DISTANCE + p.z))

quantizeshift(p::SVector{2})::Tuple{Integer,Integer} =
    WIDTH ÷ 2 + round(Int, p.x * ASCII_RATIO), HEIGHT ÷ 2 - round(Int, p.y)

function normalize(v::SVector{3})::Vec3
    l = length(v)
    return (l != 0) ? v / l : v
end

length(v::SVector{3})::Real = sqrt(dot(v, v))

main()

