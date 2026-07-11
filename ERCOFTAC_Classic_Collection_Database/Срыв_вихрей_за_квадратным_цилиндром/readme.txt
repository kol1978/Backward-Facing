 В этом файле README описаны отформатированные экспериментальные данные о
 течении Денни Лин вокруг квадратного цилиндра.
 =================================================================

 Все данные нормализованы с использованием длины ребра цилиндра H и
свободной скорости набегающего потока U.

 Структура данных
 =====================

 Файлы с данными расположены в 21 каталоге. На одной из них представлены усредненные по времени данные, а на остальных — усредненные по фазе данные при 20 фазовых углах в течение цикла вихреобразования.

 phase_av : усредненные по времени данные
 phase_01: усредненные по фазе данные при 0 градусах
 phase_02: усредненные по фазе данные при 18 градусах
 phase_03 : усредненные по фазе данные при 36 градусах
 фаза_04: усредненные по фазе данные при 54 градусах
 фаза_05: усредненные по фазе данные при 72 градусах
 фаза_06: усредненные по фазе данные при 90 градусах
 фаза_07: усредненные по фазе данные при 108 градусах
 фаза_08: усредненные по фазе данные при 126 градусах
 фаза_09: усредненные по фазе данные при 144 градусах
 фаза_10: усредненные по фазе данные при 162 градусах
 фаза_11: усредненные по фазе данные при 180 градусах
 фаза_12: усредненные по фазе данные при 198 градусах
 фаза_13 : Усредненные данные по фазе при 216 градусах
 фаза_14: усредненные по фазе данные при 234 градусах
 фаза_15: усредненные по фазе данные при 252 градусах
 фаза_16: усредненные по фазе данные при 270 градусах
 фаза_17: усредненные по фазе данные при 288 градусах
 фаза_18: усредненные по фазе данные при 306 градусах
 фаза_19: усредненные по фазе данные при 324 градусах
 phase_20 : Усредненные по фазе данные при 342 градусах

 Внутри каждого каталога файлы именуются в соответствии со структурой:
 a_bxxxx_pos.dat

 "a" указывает, измеряются ли один или два компонента:
 "a" = o: один компонент
 t: два компонента

 "bxxx" указывает местоположение профиля x:
 "b" = d : ниже по потоку от центра цилиндра
 u : выше по потоку от центра цилиндра
 "xxxx" = 1000 * x / Ч

 "pos" указывает местоположение фазы:
 "avg" = среднее значение: для значений, усредненных по времени
 p01: данные фазы 1
 ...
 p20: данные фазы 20


 Каждый файл имеет заголовок таблицы, объясняющий содержимое файла.

Пример:
---------------------------
## filename = Data/phase_08/t_d0750_p08.dat
##--------------------------------------
## 8 ( = номер фазы --> фазовый угол 126 градусов )
##-------------------------------------------------------------------------
## x/H y/H Umean/U Vmean/U u'/U v'/U u'v'/U^2 fff_u fff_v
##=========================================================================
# 8 ( = количество точек данных )
 0.7500 0.7500 1.305 -0.357 0.275 0.215 -3.56e-02 1.00 0.07
 0.7500 0.8750 1.407 -0.316 0.118 0.133 -1.50e-03 1.00 0.03
 ........
---------------------------
===================================================================================
   This README-files describes the formatted experimental Data of
   Denny Lyn's Flow around a square cylinder.
   =================================================================

 All the data are normalized, using the cylinder's edge length, H, and
the upstream free velocity, U.

   Structure of the data
   =====================

 Tha datafiles are arranged in 21 directories. One contains the time-averaged 
data, while the rest contain phase-averaged data at 20 phase angles through the 
vortex-shedding cycle.

 phase_av : Time-averaged data
 phase_01 : Phase averaged data at 0 degrees
 phase_02 : Phase averaged data at 18 degrees
 phase_03 : Phase averaged data at 36 degrees
 phase_04 : Phase averaged data at 54 degrees
 phase_05 : Phase averaged data at 72 degrees
 phase_06 : Phase averaged data at 90 degrees
 phase_07 : Phase averaged data at 108 degrees
 phase_08 : Phase averaged data at 126 degrees
 phase_09 : Phase averaged data at 144 degrees
 phase_10 : Phase averaged data at 162 degrees
 phase_11 : Phase averaged data at 180 degrees
 phase_12 : Phase averaged data at 198 degrees
 phase_13 : Phase averaged data at 216 degrees
 phase_14 : Phase averaged data at 234 degrees
 phase_15 : Phase averaged data at 252 degrees
 phase_16 : Phase averaged data at 270 degrees
 phase_17 : Phase averaged data at 288 degrees
 phase_18 : Phase averaged data at 306 degrees
 phase_19 : Phase averaged data at 324 degrees
 phase_20 : Phase averaged data at 342 degrees

 Within each directory, files are named by the structure:
   a_bxxxx_pos.dat

   "a" indicates whether one or two component measurements:
     "a"= o : one component
          t : two component

   "bxxx" indicates the x location of the profile:
     "b" =d : downstream of cylinder centre
          u : upstream of cylinder centre
     "xxxx" = 1000 * x/H

   "pos" indicates the phase-location:
     "avg" =  avg :  for time-averaged values
              p01 :  phase 1 data
              ...
              p20 :  phase 20 data

  
  Each file has a table-header, explaining the contents of the file.

Example:
---------------------------
## filename = Data/phase_08/t_d0750_p08.dat
##--------------------------------------
## 8 ( = phase number --> phaseangle 126 degree )
##-------------------------------------------------------------------------
##   x/H     y/H   Umean/U  Vmean/U  u'/U    v'/U   u'v'/U^2   fff_u  fff_v
##=========================================================================
# 8  ( = number of datapoints )
   0.7500   0.7500   1.305  -0.357   0.275   0.215 -3.56e-02   1.00   0.07
   0.7500   0.8750   1.407  -0.316   0.118   0.133 -1.50e-03   1.00   0.03
 ........
---------------------------
